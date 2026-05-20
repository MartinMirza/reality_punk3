#pragma once

#include "common.h"
#include "keyboard.h"
#include "rp3_string.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
    #define GAME_EXPORT __declspec(dllexport)
#else
    #define GAME_EXPORT __attribute__((visibility("default")))
#endif

struct WindowSettings
{
    u32 width;
    u32 height;
    RP3String title;
};

using RenderCommandBuffer = void*;

struct GameMemory
{
    void* persistent_storage; // Persistent game state (e.g., assets, entities)
    void* transient_storage; // Temporary memory (reset every frame)
    RenderCommandBuffer* renderCommandBuffer;
    
    void* gameState = nullptr;
    bool requestReinit = false;
};

struct GameWindowSettings
{
    u32 width;
    u32 height;
};

using GameInitFunc = void(*)(GameMemory&, GameWindowSettings&);
using GameUpdateAndRenderFunc = void(*)(GameMemory&, GameWindowSettings&, KeyboardState&, float delta_time);

// The ONLY exported symbol from the game DLL
struct GameAPI
{
    GameInitFunc game_init;
    GameUpdateAndRenderFunc update_and_render;
};
