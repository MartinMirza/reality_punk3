#pragma once
#include <string_view>

#include "common.h"

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
    std::string_view title;
};