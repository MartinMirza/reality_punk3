#pragma once
#include "common.h"

struct HashEntry
{
    bool occupied { false };
    HashEntry* next { nullptr };
    u8* data { nullptr };
};

using HashFunc = u32(*)(void* key);
using CompareFunc = bool(*)(void* key1, void* key2);

struct Hashmap
{
    
    HashEntry* table;
};
