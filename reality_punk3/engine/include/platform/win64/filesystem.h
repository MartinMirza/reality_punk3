#pragma once
#include "core/common.h"
#include "core/memory.h"
#include "core/rp3_string.h"

// Forward declare Windows types we use (avoid including windows.h in header)
using HANDLE = void*;

using FileHandle = u64;

// Access modes for file mapping
enum class FileAccess : u32
{
    READ,
    WRITE,
    READ_WRITE,
};

// Creation disposition for file opening (RP3_ prefix to avoid WinAPI collision)
enum class FileCreation : u32
{
    RP3_OPEN_EXISTING,
    RP3_CREATE_NEW,
    RP3_OPEN_ALWAYS,
    RP3_CREATE_ALWAYS,
    RP3_TRUNCATE_EXISTING,
};

// View flags
enum class FileViewFlags : u32
{
    NONE = 0,
    READ_ONLY = 1 << 0,
    WRITEABLE = 1 << 1,
};

// Memory-mapped view of a file
struct FileView
{
    void* data;             // Pointer to OS memory-mapped file content
    size_t size;            // Current committed size (actual file size on disk)
    size_t capacity;        // Total reserved address space
    FileViewFlags flags;    // View flags (readable/writable)
};

// Memory-mapped file handle
struct File
{
    FileHandle file_handle;    // Win32 file handle (i32)
    HANDLE section_handle;     // NT section handle or WinAPI mapping handle
    FileView view;            // Mapped view (view.data points to OS-mapped memory)
    bool owns_file_handle;    // Whether we own the file handle (and should close it)
    bool is_nt_section;       // Whether using NT functions (for resizing support)
};

// Function declarations
// Stack-based API (caller provides File storage on stack)
bool File_Open(File& file, RP3String path, FileAccess access, FileCreation creation, size_t reserve_size);
bool File_Close(File& file);
bool File_Resize(File& file, size_t new_size);
bool File_Flush(File& file);
bool File_CreateReadOnlyView(const File& file, FileView& out_readonly_view);

// Heap-based API (File struct allocated via Allocator)
File* File_Create(Allocator& allocator, RP3String path, FileAccess access, FileCreation creation, size_t reserve_size);
void File_Destroy(Allocator& allocator, File& file);
