#include "platform/win64/filesystem.h"

#include <windows.h>

// NT status codes
#define STATUS_SUCCESS 0

// SECTION_INHERIT enum
enum SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2,
};

// NT function types - using instead of typedef
using NtCloseFn = NTSTATUS (NTAPI *)(HANDLE);
using NtCreateSectionFn = NTSTATUS (NTAPI *)(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN void* ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG PageAttributes,
    IN ULONG SectionAttributes,
    IN HANDLE FileHandle OPTIONAL
);
using NtMapViewOfSectionFn = NTSTATUS (NTAPI *)(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID* BaseAddress OPTIONAL,
    IN ULONG_PTR ZeroBits OPTIONAL,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType OPTIONAL,
    IN ULONG Protect
);
using NtUnmapViewOfSectionFn = NTSTATUS (NTAPI *)(IN HANDLE ProcessHandle, IN PVOID BaseAddress);
using NtExtendSectionFn = NTSTATUS (NTAPI *)(IN HANDLE SectionHandle, IN PLARGE_INTEGER NewSectionSize);

// NTDLL function pointers (static, loaded once)
static NtCloseFn NtClose = nullptr;
static NtCreateSectionFn NtCreateSection = nullptr;
static NtMapViewOfSectionFn NtMapViewOfSection = nullptr;
static NtUnmapViewOfSectionFn NtUnmapViewOfSection = nullptr;
static NtExtendSectionFn NtExtendSection = nullptr;

// Section attributes
#define RP3_SEC_RESERVE 0x4000000

static bool LoadNTDLLFunctions()
{
    HMODULE ntdll = GetModuleHandleA("NTDLL.DLL");
    if (ntdll == nullptr)
    {
        return false;
    }

    NtClose = (NtCloseFn)GetProcAddress(ntdll, "NtClose");
    NtCreateSection = (NtCreateSectionFn)GetProcAddress(ntdll, "NtCreateSection");
    NtMapViewOfSection = (NtMapViewOfSectionFn)GetProcAddress(ntdll, "NtMapViewOfSection");
    NtUnmapViewOfSection = (NtUnmapViewOfSectionFn)GetProcAddress(ntdll, "NtUnmapViewOfSection");
    NtExtendSection = (NtExtendSectionFn)GetProcAddress(ntdll, "NtExtendSection");

    if (NtClose == nullptr || NtCreateSection == nullptr || NtMapViewOfSection == nullptr ||
        NtUnmapViewOfSection == nullptr || NtExtendSection == nullptr)
    {
        return false;
    }

    return true;
}

static void EnsureNTDLLLoaded()
{
    static bool loaded = LoadNTDLLFunctions();
    (void)loaded;
}

bool File_Open(RP3String path, FileAccess access, FileCreation creation, size_t reserve_size, RP3File& out_file)
{
    if (path.buffer == nullptr)
    {
        return false;
    }

    memset(&out_file, 0, sizeof(out_file));

    EnsureNTDLLLoaded();

    // Map FileAccess to Win32 flags
    DWORD win32_access = 0;
    DWORD win32_creation = 0;
    ACCESS_MASK section_access = 0;
    DWORD page_protection = 0;

    switch (access)
    {
        case FileAccess::READ:
        {
            win32_access = GENERIC_READ;
            section_access = SECTION_MAP_READ;
            page_protection = PAGE_READONLY;
            break;
        }
        case FileAccess::WRITE:
        {
            win32_access = GENERIC_WRITE;
            section_access = SECTION_MAP_WRITE;
            page_protection = PAGE_READWRITE;
            break;
        }
        case FileAccess::READ_WRITE:
        {
            win32_access = GENERIC_READ | GENERIC_WRITE;
            section_access = SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_EXTEND_SIZE;
            page_protection = PAGE_READWRITE;
            break;
        }
    }

    switch (creation)
    {
        case FileCreation::RP3_OPEN_EXISTING:
        {
            win32_creation = OPEN_EXISTING;
            break;
        }
        case FileCreation::RP3_CREATE_NEW:
        {
            win32_creation = CREATE_NEW;
            break;
        }
        case FileCreation::RP3_OPEN_ALWAYS:
        {
            win32_creation = OPEN_ALWAYS;
            break;
        }
        case FileCreation::RP3_CREATE_ALWAYS:
        {
            win32_creation = CREATE_ALWAYS;
            break;
        }
        case FileCreation::RP3_TRUNCATE_EXISTING:
        {
            win32_creation = TRUNCATE_EXISTING;
            break;
        }
    }

    // Create file handle
    HANDLE win32_handle = CreateFileA(
        path.buffer,
        win32_access,
        FILE_SHARE_READ,
        nullptr,
        win32_creation,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (win32_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    out_file.file_handle = (FileHandle)(uptr)win32_handle;
    out_file.owns_file_handle = true;
    out_file.is_nt_section = (access != FileAccess::READ);

    // Get current file size
    LARGE_INTEGER file_size_large;
    if (GetFileSizeEx(win32_handle, &file_size_large) == 0)
    {
        CloseHandle(win32_handle);
        return false;
    }
    size_t file_size = (size_t)file_size_large.QuadPart;

    // For read-write with resizing support, use NT functions
    if (out_file.is_nt_section)
    {
        LARGE_INTEGER section_size;
        section_size.QuadPart = reserve_size > 0 ? reserve_size : 1;

        HANDLE section = nullptr;
        NTSTATUS status = NtCreateSection(
            &section,
            section_access,
            nullptr,
            &section_size,
            page_protection,
            RP3_SEC_RESERVE,
            win32_handle
        );

        if (status != STATUS_SUCCESS)
        {
            CloseHandle(win32_handle);
            return false;
        }

        out_file.section_handle = section;

        size_t view_size = file_size;
        void* data = nullptr;

        LARGE_INTEGER section_offset = { };
        status = NtMapViewOfSection(
            section,
            GetCurrentProcess(),
            &data,
            0,
            file_size,
            &section_offset,
            &view_size,
            ViewUnmap,
            MEM_RESERVE,
            page_protection
        );

        if (status != STATUS_SUCCESS)
        {
            NtClose(section);
            CloseHandle(win32_handle);
            return false;
        }

        out_file.view.data = data;
        out_file.view.size = file_size;
        out_file.view.capacity = view_size;
        out_file.view.flags = FileViewFlags::WRITEABLE;
    }
    else
    {
        // Use standard WinAPI for read-only or write-only
        DWORD mapping_protection = (page_protection == PAGE_READONLY) ? PAGE_READONLY : PAGE_READWRITE;
        HANDLE mapping = CreateFileMappingA(
            win32_handle,
            nullptr,
            mapping_protection,
            0,
            0,
            nullptr
        );

        if (mapping == nullptr)
        {
            CloseHandle(win32_handle);
            return false;
        }

        DWORD map_access = (page_protection == PAGE_READONLY) ? FILE_MAP_READ : FILE_MAP_WRITE;
        void* data = MapViewOfFile(mapping, map_access, 0, 0, 0);

        if (data == nullptr)
        {
            CloseHandle(mapping);
            CloseHandle(win32_handle);
            return false;
        }

        out_file.section_handle = mapping;
        out_file.is_nt_section = false;
        out_file.view.data = data;
        out_file.view.size = file_size;
        out_file.view.capacity = file_size;
        out_file.view.flags = (access != FileAccess::READ) ? FileViewFlags::WRITEABLE : FileViewFlags::READ_ONLY;
    }

    return true;
}

bool File_Close(RP3File& file)
{
    // Flush writes to disk
    if (((u32)file.view.flags & (u32)FileViewFlags::WRITEABLE) && file.view.data != nullptr)
    {
        FlushViewOfFile(file.view.data, file.view.size);
    }

    // Unmap the OS memory-mapped view
    if (file.view.data != nullptr)
    {
        if (file.is_nt_section)
        {
            NtUnmapViewOfSection(GetCurrentProcess(), file.view.data);
        }
        else
        {
            UnmapViewOfFile(file.view.data);
        }
        file.view.data = nullptr;
    }

    // Close section/mapping handle
    if (file.section_handle != nullptr)
    {
        if (file.is_nt_section)
        {
            NtClose(file.section_handle);
        }
        else
        {
            CloseHandle(file.section_handle);
        }
        file.section_handle = nullptr;
    }

    // Close file handle
    if (file.owns_file_handle && file.file_handle != (i32)INVALID_HANDLE_VALUE)
    {
        CloseHandle((HANDLE)(uptr)file.file_handle);
        file.file_handle = (i32)INVALID_HANDLE_VALUE;
    }

    file.view.size = 0;
    file.view.capacity = 0;
    file.view.flags = FileViewFlags::NONE;
    file.owns_file_handle = false;
    file.is_nt_section = false;

    return true;
}

bool File_ExtendSection(RP3File& file, size_t new_size)
{
    if (file.is_nt_section == false)
    {
        return false;
    }

    if (new_size <= file.view.size)
    {
        return false;
    }

    // Extend section - this commits new pages and grows the file on disk
    LARGE_INTEGER new_size_li;
    new_size_li.QuadPart = new_size;

    NTSTATUS status = NtExtendSection(file.section_handle, &new_size_li);

    if (status != STATUS_SUCCESS)
    {
        return false;
    }

    file.view.size = new_size;
    return true;
}

bool File_Flush(RP3File& file)
{
    if (file.view.data == nullptr)
    {
        return false;
    }

    if (((u32)file.view.flags & (u32)FileViewFlags::WRITEABLE) == 0)
    {
        return false;
    }

    return FlushViewOfFile(file.view.data, file.view.size) != 0;
}

void* File_GetData(RP3File& file)
{
    return file.view.data;
}

size_t File_GetSize(RP3File& file)
{
    return file.view.size;
}

size_t File_GetCapacity(RP3File& file)
{
    return file.view.capacity;
}

bool File_Write(RP3File &file, size_t offset, const void *data, size_t size)
{
    if (data == nullptr)
    {
        return false;
    }
    
    const size_t required_size = offset + size;
    
    if (required_size > file.view.capacity)
    {
        return false;
    }
    
    if (required_size <= file.view.capacity)
    {
        memcpy((u8*)file.view.data + offset, data, size);
        file.view.size += size;
        return true;
    }
    
    if (File_ExtendSection(file, required_size))
    {
        memcpy((u8*)file.view.data + offset, data, size);
        file.view.size += size;
        return true;
    }
    
    return false;
}

bool File_Append(RP3File& file, const void* data, size_t size)
{
    return File_Write(file, file.view.size, data, size);
}

bool File_WriteString(RP3File& file, size_t offset, RP3String str)
{
    if (str.buffer == nullptr)
    {
        return false;
    }
    return File_Write(file, offset, str.buffer, strlen(str.buffer));
}

bool File_AppendString(RP3File& file, RP3String str)
{
    return File_WriteString(file, file.view.size, str);
}