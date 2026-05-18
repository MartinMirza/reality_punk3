#pragma once
#include <Windows.h>

#include "core/keyboard.h"


Keyboard Win64_PollKeyboardKey(WPARAM vk, LPARAM lParam);
void Win64_UpdateKeyState(Keyboard key, bool is_down, KeyboardState* p_kb) ;