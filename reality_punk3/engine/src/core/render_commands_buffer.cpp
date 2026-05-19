#include "core/render_commands_buffer.h"

#include <cassert>

#include "core/game_interface.h"
#include "core/memory.h"

RenderCommandQueue* RenderCommands_CreateRCQueue(Allocator& allocator)
{
    
    RenderCommandQueue* rc_buff = (RenderCommandQueue*)allocator.alloc(allocator.ctx, sizeof(RenderCommandQueue), alignof(RenderCommandQueue));
    assert(rc_buff != nullptr);
    
    static RenderCommand dummy {};
    rc_buff->head = &dummy;
    rc_buff->tail = &dummy;
}

RenderCommand* RenderCommands_CreateRenderCommand(Allocator& allocator, RenderCommandType type, void *data)
{
    RenderCommand* cmd = (RenderCommand*)allocator.alloc(allocator.ctx, sizeof(RenderCommand), alignof(RenderCommand));
    assert(cmd != nullptr);
    cmd->type = type;
    cmd->data = data;
    
    return cmd;
}

void RenderCommands_PushCommand(RenderCommandQueue& queue, RenderCommand &command)
{
    RenderCommand* old_tail = queue.tail;
    assert(old_tail != nullptr);
    queue.tail = &command;
    old_tail->next = &command;
}

#if PLATFORM_WINDOWS
void RenderCommands_PushCommandInterlocked(RenderCommandQueue &queue, RenderCommand &command)
{
    RenderCommand* prev = (RenderCommand*)InterlockedExchangePointer((void**)&queue.tail, &command);
    prev->next= &command;
}
#endif


