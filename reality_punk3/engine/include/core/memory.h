#pragma once
#include "common.h"

struct Allocator
{
    void* ctx;
    void* (*alloc)(void* ctx, size_t size, size_t alignment);
    void* (*realloc)(void* ctx, void* ptr, size_t size, size_t alignment);
    void (*free)(void* ctx, void* ptr);
};

struct Arena
{
    u8* buffer;
    size_t capacity;     
    size_t offset;
};

Allocator Memory_MakeDefaultAllocator();
Allocator Memory_MakeLinearArenaAllocator(Arena& arena);

void* Arena_LinearAlloc(void* ctx, size_t size, size_t alignment);
void* Arena_LinearAllocWrap(Arena& arena, size_t size, size_t alignment);
inline void* Arena_Realloc(void* ctx, void* ptr, size_t size, size_t alignment) { return nullptr; }
inline void Arena_Free(void* ctx, void* ptr) {};


uptr Memory_AlignUp(uptr address_before_alignment, size_t alignment);
uptr Memory_GetAlignedOffset(u8* buffer, size_t offset, size_t alignment);