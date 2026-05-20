#include "core/logging.h"

#include <cassert>

#include "core/game_interface.h"
#include "platform/win64/filesystem.h"

constexpr size_t NodeBufferSize { 256 };
struct Node
{
    StringView message { };
    LevelType leveType { };
    
    Node* next { nullptr };
};

//TODO(MM): zamienic globale na pointery
static struct State
{
    Node* head { nullptr };
    Node* tail { nullptr };
} gState;

static struct LogState
{
    SinkConfig* configs { nullptr };
    u32 configCount { 0 };
    
    HANDLE fileHandle { nullptr };
    HANDLE consoleOutputHandle { nullptr };
    HANDLE consoleErrorHandle { nullptr };
    
    
} gLogState;

static void Queue_Init()
{
    static Node dummy {};
    gState.head = &dummy;
    gState.tail = &dummy;
}

static void Queue_Push(Node* n)
{
    assert(n != nullptr && gState.tail != nullptr);

    Node* prev = static_cast<Node*>(InterlockedExchangePointer(reinterpret_cast<void**>(&gState.tail), n));
    prev->next = n;
} 

static thread_local Allocator gAllocator;
static thread_local StringBuilder gBuilder;

void Logging_CreateLogState(Allocator &allocator, const SinkInitializerList &config_init_list)
{
    Queue_Init();
    gLogState.configs = (SinkConfig*)allocator.alloc(allocator.ctx, sizeof(SinkConfig) * config_init_list.count, alignof(SinkConfig));
    memcpy(gLogState.configs, config_init_list.configs, sizeof(SinkConfig) * config_init_list.count);

    for (u32 i = 0; i < config_init_list.count; i++)
    {
        const SinkConfig& config = config_init_list.configs[i];
        switch (config.type)
        {
            case SinkType::FILE:
            {
                //gLogState.fileHandle = CreateFileA(config.file.filePath, GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            }
                break;
            case SinkType::CONSOLE:
            {
                const HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                if (output_handle == INVALID_HANDLE_VALUE)
                {
                    AllocConsole();
                }

                gLogState.consoleOutputHandle = output_handle;
                gLogState.consoleErrorHandle = GetStdHandle(STD_ERROR_HANDLE);
            }
                break;
            case SinkType::DEBUGGER:
                break;
            case SinkType::NONE:
                break;
        }
    }
}
void Logging_InitializeThreadLocal(Allocator& thread_local_allocator)
{
    gAllocator = thread_local_allocator;
    gBuilder = StringBuilder_Create(gAllocator, KB);
}

void Logging_Write(LevelType level, RP3String str)
{
    const char* p_start = gBuilder.buffer + gBuilder.offset;
    StringBuilder_Append(gBuilder, str);
    StringBuilder_Append(gBuilder, "\n");
    
    Node* node = (Node*)gAllocator.alloc(gAllocator.ctx, sizeof(Node), alignof(Node));
    node->message  = StringView_From({ p_start });
    node->leveType = level;
}

void Logging_DestroyLogState()
{
    
}

void Logging_FlushLogState(Allocator &allocator)
{
    StringBuilder b = StringBuilder_Create(allocator, 3 * KB);
    while (true)
    {
        if (const Node* curr = gState.head)
        {
            if (curr->message.length > 0)
            {
                StringBuilder_Append(b, curr->message);
            }
            
            if (gState.head == gState.tail)
            {
                Queue_Init();
                break;
            }
            
            if (curr->next != nullptr)
            {
                allocator.free(allocator.ctx, (void*)curr);
                gState.head = curr->next;
            }
        }
    }
    
    //let's explicitly null terminate this fucker
    StringBuilder_Append(b, '\0');
    const RP3String output { b.buffer };
    
    for (size_t i = 0; i < 1; i++)
    {
        switch (gLogState.configs[i].type)
        {
            case SinkType::NONE:
                break;
            case SinkType::FILE:
                const SinkConfig& c = gLogState.configs[i];
                RP3File file {};
                if (File_Open(c.file.path, FileAccess::WRITE, FileCreation::RP3_OPEN_ALWAYS, 0, file))
                {
                    File_WriteString(file, File_GetSize(file), output);
                    File_Close(file);
                }
                break;
            case SinkType::CONSOLE:
                break;
            case SinkType::DEBUGGER:
                break;
        }
    }
}



