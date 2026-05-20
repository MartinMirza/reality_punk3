#include "core/memory.h"

#include <cassert>
#include <cstdlib>

Allocator Memory_CreateDefaultAllocator()
{
    Allocator allocator { };
    allocator.ctx = nullptr;
    allocator.alloc = Memory_DefaultAlloc;
    allocator.realloc = Memory_DefaultRealloc;
    allocator.free = Memory_DefaultFree;
    
    return allocator;
}

Allocator Memory_CreateLinearArenaAllocator(Arena &arena)
{
    Allocator allocator { };
    allocator.ctx = (void*)&arena;
    allocator.alloc = Arena_LinearAlloc;
    allocator.realloc = Arena_Realloc;
    allocator.free = Arena_Free;
    
    return allocator;
}

void* Memory_DefaultAlloc(void *ctx, size_t size, size_t alignment)
{
     return malloc(size);   
}
void* Memory_DefaultRealloc(void *ctx, void *ptr, size_t size, size_t alignment)
{
    return realloc(ptr, size);
}

void Memory_DefaultFree(void *ctx, void *ptr)
{
    free(ptr);
}

void* Arena_LinearAlloc(void *ctx, const size_t size, const size_t alignment)
{
    Arena* a = (Arena*)ctx;

    const uptr aligned_offset = Memory_GetAlignedOffset(a->buffer, a->offset, alignment);
    if (aligned_offset + size <= a->capacity)
    {
        u8* result = a->buffer + aligned_offset;
        a->offset = aligned_offset + size;
        
        return result;
    }

    return nullptr;
}

void* Arena_LinearAllocWrap(void* ctx, const size_t size, const size_t alignment)
{
    Arena* a = (Arena*)ctx;
    
    if (size > a->capacity)
    {
        return nullptr;
    }

    uptr aligned_offset = Memory_GetAlignedOffset(a->buffer, a->offset, alignment);
    if (aligned_offset + size > a->capacity)
    {
        a->offset = 0;
        aligned_offset = Memory_GetAlignedOffset(a->buffer, a->offset, alignment);
        
        if (aligned_offset + size > a->capacity)
        {
            return nullptr;
        }
    }
    
    u8* result = a->buffer + aligned_offset;
    a->offset = aligned_offset + size;
    
    return result;
}

uptr Memory_GetAlignedOffset(u8* buffer, size_t offset, size_t alignment)
{
    const uptr address_before_alignment = (uptr)(&buffer[offset]);
    const uptr aligned_address = Memory_AlignUp(address_before_alignment, alignment);
    return aligned_address - (uptr)(buffer);
}
	
uptr Memory_AlignUp(uptr address_before_alignment, size_t alignment) 
{
    const uptr is_power_of_two = (alignment & (alignment - 1)) == 0;
    assert(is_power_of_two);
    const uptr modulo = address_before_alignment & (alignment - 1);

    if (modulo == 0)
    {
        return address_before_alignment;
    }

    return address_before_alignment += alignment - modulo;
}
