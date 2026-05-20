#pragma once
#include "memory.h"

//guaranteed to be null terminated
struct RP3String
{
    const char* buffer { nullptr };
};

//not guaranteed to be null terminated
struct StringView
{
    const char* buffer { nullptr };
    size_t length { 0 };
};

struct StringBuilder
{
    //buffer is not null terminated unless it was done explicitly!
    //so you can just append by '\0' to make it string or wrap into StringView grabbing offset
    char* buffer { nullptr };
    size_t offset { 0 };
    size_t capacity { 0 };
};

StringView StringView_From(RP3String string);
StringView StringView_ChopLeft(StringView view, u64 num_chopped);
StringView StringView_ChopRight(StringView view, u64 num_chopped);
bool StringView_Split (char delimiter, StringView view, StringView& out_first, StringView& out_second);

RP3String String_Create(Allocator& allocator, const char* string);
RP3String String_Copy(Allocator& allocator, RP3String string);
RP3String String_Concatenate(Allocator& allocator, RP3String string1, RP3String string2);
RP3String String_Concatenate(Allocator& allocator, RP3String string1, StringView string2);
RP3String String_Concatenate(Allocator& allocator, StringView string1, RP3String string2);
RP3String String_Concatenate(Allocator& allocator, StringView string1, StringView string2);
RP3String String_FromView(Allocator& allocator, StringView view);
StringBuilder StringBuilder_Create(Allocator& allocator, size_t size);
bool StringBuilder_Append(StringBuilder& builder, RP3String string);
bool StringBuilder_Append(StringBuilder& builder, StringView string);
bool StringBuilder_Append(StringBuilder& builder, const char* string);
bool StringBuilder_Append(StringBuilder& builder, i32 number);
bool StringBuilder_Append(StringBuilder& builder, float number);

