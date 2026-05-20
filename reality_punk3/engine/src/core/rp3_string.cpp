#include "core/rp3_string.h"

#include <cassert>
#include <cstring>
#include <cstdio>

StringView StringView_From(const RP3String string)
{
    return {.buffer = string.buffer, .length = strlen(string.buffer) };
}

StringView StringView_ChopLeft(const StringView view, const u64 num_chopped)
{
    assert(num_chopped < view.length);
    return {.buffer = view.buffer, .length = view.length - num_chopped };
}

StringView StringView_ChopRight(const StringView view, const u64 num_chopped)
{
    assert(num_chopped < view.length);
    return {.buffer = view.buffer + num_chopped, .length = view.length - num_chopped };
}
bool StringView_Split(const char delimiter, const StringView view, StringView& out_first, StringView& out_second)
{
    for (u32 i = 0; i < view.length; i++)
    {
        if (view.buffer[i] == delimiter)
        {
            out_first = StringView_ChopLeft(view, view.length - i);
            out_second = StringView_ChopRight(view, i + 1);
            return true;
        }
    }
    
    return false;
}

RP3String String_Create(Allocator& allocator, const char* string)
{
    const size_t length = strlen(string);
    void* mem = allocator.alloc(allocator.ctx, length, alignof(char));
    
    memcpy(mem, string, length);
    
    return { .buffer = (char*)mem };
}

inline RP3String String_Copy(Allocator& allocator, const RP3String string)
{
    return String_Create(allocator, string.buffer);
}

static char* AllocateConcatedString(Allocator& allocator, const char* string1, const char* string2, const size_t length1, const size_t length2, const size_t total_length)
{
    char* mem = (char*)allocator.alloc(allocator.ctx, total_length + 1, alignof(char));
    assert(mem != nullptr); // i should raise a critical here
    memcpy(mem, string1, length1);
    memcpy(mem + length1, string2, length2);
    mem[total_length] = '\0';
    return mem;
}

RP3String String_Concatenate(Allocator& allocator, const RP3String string1, const RP3String string2)
{
    const size_t length1 = strlen(string1.buffer);
    const size_t length2 = strlen(string2.buffer);
    const size_t total_length = length1 + length2;
    
    return { .buffer = AllocateConcatedString(allocator, string1.buffer, string2.buffer, length1, length2, total_length) };
}

RP3String String_Concatenate(Allocator& allocator, RP3String string1, StringView string2)
{
    const size_t length1 = strlen(string1.buffer);
    const size_t length2 = string2.length;
    const size_t total_length = length1 + length2;
    
    return { .buffer = AllocateConcatedString(allocator, string1.buffer, string2.buffer, length1, length2, total_length) };    
}

RP3String String_Concatenate(Allocator& allocator, const StringView string1, const RP3String string2)
{
    const size_t length1 = string1.length;
    const size_t length2 = strlen(string2.buffer);
    const size_t total_length = length1 + length2;
    
    return { .buffer = AllocateConcatedString(allocator, string1.buffer, string2.buffer, length1, length2, total_length) };
}
RP3String String_Concatenate(Allocator& allocator, const StringView string1, const StringView string2)
{
    const size_t length1 = string1.length;
    const size_t length2 = string2.length;
    const size_t total_length = length1 + length2;
    
    return { .buffer = AllocateConcatedString(allocator, string1.buffer, string2.buffer, length1, length2, total_length) };
}
RP3String String_FromView(Allocator& allocator, StringView view)
{
    char* mem = (char*)allocator.alloc(allocator.ctx, view.length + 1, alignof(char));
    assert(mem != nullptr);
    
    memcpy(mem, view.buffer, view.length);
    mem[view.length] = '\0';
    
    return { .buffer = mem };
}

StringBuilder StringBuilder_Create(Allocator& allocator, size_t size)
{
    StringBuilder b {};
    b.buffer = (char*)allocator.alloc(allocator.ctx, size, alignof(char));
    b.capacity = size;
    return b;
}

bool StringBuilder_Append(StringBuilder& builder, RP3String string)
{
    return StringBuilder_Append(builder, StringView_From(string));
}

bool StringBuilder_Append(StringBuilder& builder, StringView string)
{
    if (builder.buffer != nullptr && string.buffer != nullptr)
    {
        memcpy(builder.buffer, string.buffer, string.length);
        builder.offset += string.length; 
        
        return true;
    }
    
    return false;
}

bool StringBuilder_Append(StringBuilder &builder, const char* string)
{
    return StringBuilder_Append(builder, StringView_From( {string }));
}

bool StringBuilder_Append(StringBuilder& builder, i32 number)
{
    char scratch[10];
    if (sprintf_s(scratch, "%d", number))
    {
        memcpy(builder.buffer + builder.offset, scratch, strlen(scratch));
        return true;
    }
    
    return false;
}

bool StringBuilder_Append(StringBuilder &builder, float number)
{
    char scratch[10];
    if (sprintf_s(scratch, "%f", number))
    {
        memcpy(builder.buffer + builder.offset, scratch, strlen(scratch));
        return true;
    }
    
    return false;
}
















