#pragma once

#include "Common.h"

enum class Keyboard : u16
{
    // Numbers
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,

    // Letters
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    // Modifiers
    KEY_LEFT_SHIFT,
    KEY_RIGHT_SHIFT,
    KEY_LEFT_CTRL,
    KEY_RIGHT_CTRL,
    KEY_LEFT_ALT,
    KEY_RIGHT_ALT,
    KEY_LEFT_SUPER,
    KEY_RIGHT_SUPER,

    // Navigation / editing
    KEY_TAB,
    KEY_CAPS_LOCK,
    KEY_SPACE,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_ESCAPE,
    KEY_INSERT,
    KEY_DELETE,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,

    // Punctuation / symbols
    KEY_MINUS,
    KEY_EQUALS,
    KEY_LEFT_BRACKET,
    KEY_RIGHT_BRACKET,
    KEY_BACKSLASH,
    KEY_COMMA,
    KEY_PERIOD,
    KEY_SEMICOLON,
    KEY_APOSTROPHE,
    KEY_GRAVE,
    KEY_FORWARD_SLASH,

    // Function keys
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,

    // Numpad
    KEY_NUMPAD_0,
    KEY_NUMPAD_1,
    KEY_NUMPAD_2,
    KEY_NUMPAD_3,
    KEY_NUMPAD_4,
    KEY_NUMPAD_5,
    KEY_NUMPAD_6,
    KEY_NUMPAD_7,
    KEY_NUMPAD_8,
    KEY_NUMPAD_9,
    KEY_NUMPAD_DECIMAL,
    KEY_NUMPAD_PLUS,
    KEY_NUMPAD_MINUS,
    KEY_NUMPAD_MULTIPLY,
    KEY_NUMPAD_DIVIDE,
    KEY_NUMPAD_ENTER,

    // System / special keys
    KEY_PRINT_SCREEN,
    KEY_SCROLL_LOCK,
    KEY_PAUSE,
    KEY_MENU,

    KEY_COUNT // must always be last
};

struct KeyboardState
{
    u8 pressed[(u16)Keyboard::KEY_COUNT];
    u8 up[(u16)Keyboard::KEY_COUNT];
    u8 down[(u16)Keyboard::KEY_COUNT];
};

inline bool Keyboard_KeyPressed(const KeyboardState* p_keyboard, Keyboard key)
{
    return p_keyboard->pressed[(u16)key];
}

inline bool Keyboard_IsKeyDown(const KeyboardState* p_keyboard, Keyboard key)
{
    return p_keyboard->down[(u16)key];
}

inline bool Keyboard_IsKeyUp(const KeyboardState* p_keyboard, Keyboard key)
{
    return p_keyboard->up[(u16)key];
}
