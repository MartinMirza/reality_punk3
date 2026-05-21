#pragma once
#include "core/common.h"
#include "core/rp3_string.h"

// Forward declare Windows types we use (avoid including windows.h in header)
using HANDLE = void*;
using FileHandle = i32;

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
    void* data;
    size_t size;
    size_t capacity;
    FileViewFlags flags;
};

// Memory-mapped file handle
struct RP3File
{
    FileHandle file_handle;
    HANDLE section_handle;
    FileView view;
    bool owns_file_handle;
    bool is_nt_section;
};

// Stack-based API (caller provides File storage on stack)
bool File_Open(RP3String path, FileAccess access, FileCreation creation, size_t reserve_size, RP3File& out_file);
bool File_Close(RP3File& file);
bool File_ExtendSection(RP3File& file, size_t new_size);
bool File_Flush(RP3File& file);
void* File_GetData(RP3File& file);
size_t File_GetSize(RP3File& file);
size_t File_GetCapacity(RP3File& file);

// Convenience write functions (auto-resize)
bool File_Write(RP3File& file, size_t offset, const void* data, size_t size);
bool File_Append(RP3File& file, const void* data, size_t size);
bool File_WriteString(RP3File& file, size_t offset, RP3String str);
bool File_AppendString(RP3File& file, RP3String str);
