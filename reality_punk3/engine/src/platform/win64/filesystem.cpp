#include "filesystem.h"
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
#define SEC_RESERVE 0x4000000

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

// ============================================
// Stack-based API
// ============================================

bool File_Open(File& file, RP3String path, FileAccess access, FileCreation creation, size_t reserve_size)
{
    if (path.buffer == nullptr)
    {
        return false;
    }

    memset(&file, 0, sizeof(file));

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

    file.file_handle = (FileHandle)win32_handle;
    file.owns_file_handle = true;
    file.is_nt_section = (access == FileAccess::READ_WRITE);

    // Get current file size
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(win32_handle, &file_size) == 0)
    {
        CloseHandle(win32_handle);
        return false;
    }
    size_t current_size = (size_t)file_size.QuadPart;

    // For read-write with resizing support, use NT functions
    if (file.is_nt_section)
    {
        // Create NT section
        LARGE_INTEGER max_size;
        max_size.QuadPart = reserve_size > 0 ? reserve_size : current_size + GB;

        HANDLE section = nullptr;
        NTSTATUS status = NtCreateSection(
            &section,
            section_access,
            nullptr,
            &max_size,
            page_protection,
            SEC_RESERVE,
            win32_handle
        );

        if (status != STATUS_SUCCESS)
        {
            CloseHandle(win32_handle);
            return false;
        }

        file.section_handle = section;

        // Map view with MEM_RESERVE
        size_t view_size = reserve_size > 0 ? reserve_size : current_size + GB;
        void* data = nullptr;

        LARGE_INTEGER section_offset = { };
        status = NtMapViewOfSection(
            section,
            GetCurrentProcess(),
            &data,
            0,
            current_size,
            &section_offset,
            &view_size,
            SECTION_INHERIT::ViewUnmap,
            MEM_RESERVE,
            page_protection
        );

        if (status != STATUS_SUCCESS)
        {
            NtClose(section);
            CloseHandle(win32_handle);
            return false;
        }

        file.view.data = data;
        file.view.size = current_size;
        file.view.capacity = view_size;
        file.view.flags = FileViewFlags::WRITEABLE;
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

        file.section_handle = mapping;
        file.is_nt_section = false;
        file.view.data = data;
        file.view.size = current_size;
        file.view.capacity = current_size;
        file.view.flags = (access != FileAccess::READ) ? FileViewFlags::WRITEABLE : FileViewFlags::READ_ONLY;
    }

    return true;
}

bool File_Close(File& file)
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
    if (file.owns_file_handle && file.file_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle((HANDLE)(uptr)file.file_handle);
        file.file_handle = INVALID_HANDLE_VALUE;
    }

    file.view.size = 0;
    file.view.capacity = 0;
    file.view.flags = FileViewFlags::NONE;
    file.owns_file_handle = false;
    file.is_nt_section = false;

    return true;
}

bool File_Resize(File& file, size_t new_size)
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

bool File_Flush(File& file)
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

bool File_CreateReadOnlyView(const File& file, FileView& out_readonly_view)
{
    if (file.view.data == nullptr)
    {
        return false;
    }

    HANDLE section = file.section_handle;

    // Create a second read-only mapping of the same section
    void* readonly_data = nullptr;
    SIZE_T view_size = file.view.capacity;

    NTSTATUS status = NtMapViewOfSection(
        section,
        GetCurrentProcess(),
        &readonly_data,
        0,
        0,
        nullptr,
        &view_size,
        ViewUnmap,
        MEM_RESERVE,
        PAGE_READONLY
    );

    if (status != STATUS_SUCCESS)
    {
        return false;
    }

    out_readonly_view.data = readonly_data;
    out_readonly_view.size = file.view.size;
    out_readonly_view.capacity = file.view.capacity;
    out_readonly_view.flags = FileViewFlags::READ_ONLY;

    return true;
}

// ============================================
// Heap-based API (Allocator semantics)
// ============================================

File* File_Create(Allocator& allocator, RP3String path, FileAccess access, FileCreation creation, size_t reserve_size)
{
    // Allocate File struct using the provided allocator
    File* file = (File*)allocator.alloc(allocator.ctx, sizeof(File), alignof(File));
    if (file == nullptr)
    {
        return nullptr;
    }

    // Initialize the file - this will map the actual file content via OS
    if (File_Open(*file, path, access, creation, reserve_size) == false)
    {
        allocator.free(allocator.ctx, file);
        return nullptr;
    }

    return file;
}

void File_Destroy(Allocator& allocator, File& file)
{
    // Close the file - this unmaps OS memory and closes handles
    File_Close(file);

    // Free the File struct itself (which was allocated via allocator.alloc)
    allocator.free(allocator.ctx, &file);
}
