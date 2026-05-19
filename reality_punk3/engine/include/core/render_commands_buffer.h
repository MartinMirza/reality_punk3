#pragma once
#include "common.h"

struct Allocator;
enum class RenderCommandType : u8
{
    NONE,
    CLEAR,
    DRAW_SPRITE
}; 

struct RenderCommand
{
    RenderCommandType type { RenderCommandType::NONE };
    void* data { nullptr};
    
    RenderCommand* next { nullptr };
};

struct RenderCommandQueue
{
    RenderCommand* head { nullptr};
    RenderCommand* tail { nullptr};
};

RenderCommandQueue* RenderCommands_CreateRCQueue(Allocator& allocator);
RenderCommand* RenderCommands_CreateRenderCommand(Allocator& allocator, RenderCommandType type, void* data);
void RenderCommands_PushCommand(RenderCommandQueue& queue, RenderCommand& command);
void RenderCommands_PushCommandInterlocked(RenderCommandQueue& queue, RenderCommand& command);
