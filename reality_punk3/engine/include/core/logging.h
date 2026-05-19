#pragma once
#include "common.h"
#include "rp3_string.h"

enum class SinkType : u8
{
    NONE, 
    FILE,
    CONSOLE, 
    DEBUGGER 
};

enum class LevelType : u8
{
    TRACE,
    INFO,
    WARN,
    RP2_ERROR,
    CRITICAL,
    OFF
};

//configs have to remain POD or trivially copyable, or we would need to make CreateLogState more complex
//and I don't wanna do that
struct FileSinkConfig
{
    StringView file_path;
    bool append;
    bool flush_on_write;
};

struct DebuggerSinkConfig
{
    bool break_on_error;
    bool break_on_critical;
};

struct ConsoleSinkConfig
{
    bool color_output;
    bool flush_on_write;
};

struct SinkConfig
{
    
    SinkType type;
    union
    {
        ConsoleSinkConfig console;
        FileSinkConfig file;
        DebuggerSinkConfig debugger;
    };
};

struct SinkInitializerList
{
    size_t count;
    struct SinkConfig configs[3];
};


// //just allocate configs on the stack, they will be copied to the arena
// void Logging_CreateLogState(Allocator& allocator, const SinkInitializerList& configs);
// //make sure to call this from every thread that wants to use logging
// void Logging_InitializeThreadLocal(Mem::Arena& arena, size_t buffer_capacity);
// void Logging_Write(LevelType level, Str::View str);
// void Logging_DestroyLogState();
// void Logging_FlushLogState(Allocator& allocator);