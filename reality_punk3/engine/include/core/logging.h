#pragma once
#include "common.h"
#include "rp3_string.h"

#if 0
#define RP2_LOG_TRACE(format, ...)
#define RP2_LOG_INFO(format, ...)
#define RP2_LOG_WARN(format, ...)
#define RP2_LOG_CRITICAL(format, ...)
#define RP2_LOG_ERROR(format, ...)
#endif


#ifdef DEBUG
    #define RP2_LOG_TRACE(format, ...) do { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
    RP2::Log::Write(RP2::Log::LevelType::TRACE, Str::AsView(buffer)); \
    } while(0)

    #define RP2_LOG_INFO(format, ...) do { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
    RP2::Log::Write(RP2::Log::LevelType::INFO, Str::AsView(buffer)); \
    } while(0)

    #define RP2_LOG_WARN(format, ...) do { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
    RP2::Log::Write(RP2::Log::LevelType::WARN, Str::AsView(buffer)); \
    } while(0)
#else
    #define RP2_LOG_TRACE(format, ...)
    #define RP2_LOG_INFO(format, ...)
    #define RP2_LOG_WARN(format, ...)
#endif


// Always available
#define RP2_LOG_ERROR(format, ...) do { \
char buffer[256]; \
snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
RP2::Log::Write(RP2::Log::LevelType::RP2_ERROR, Str::AsView(buffer)); \
} while(0)

#define RP2_LOG_CRITICAL(format, ...) do { \
char buffer[256]; \
snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
RP2::Log::Write(RP2::Log::LevelType::CRITICAL, Str::AsView(buffer)); \
} while(0)

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
    RP3String path;
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
    SinkConfig configs[3];
};


// //just allocate configs on the stack, they will be copied to the arena
void Logging_CreateLogState(Allocator& allocator, const SinkInitializerList& config_init_list);
//make sure to call this from every thread that wants to use logging
void Logging_InitializeThreadLocal(Allocator& thread_local_allocator);
void Logging_Write(LevelType level, RP3String str);
void Logging_DestroyLogState();
void Logging_FlushLogState(Allocator& allocator);