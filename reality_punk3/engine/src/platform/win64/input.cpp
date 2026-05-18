#include "platform/win64/input.h"

Keyboard Win64_PollKeyboardKey(WPARAM vk, LPARAM lParam)
{
    const INT extended = (lParam & 0x01000000) != 0;

    if (VK_SHIFT)
    {
        UINT scancode = (lParam & 0x00ff0000) >> 16;
        WPARAM new_vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
        switch (new_vk)
        {
            case VK_LSHIFT: return Keyboard::KEY_LEFT_SHIFT;
            case VK_RSHIFT: return Keyboard::KEY_RIGHT_SHIFT;
            default: ;
        }
    }

    switch (vk)
    {
        // Numbers
        case '0': return Keyboard::KEY_0;
        case '1': return Keyboard::KEY_1;
        case '2': return Keyboard::KEY_2;
        case '3': return Keyboard::KEY_3;
        case '4': return Keyboard::KEY_4;
        case '5': return Keyboard::KEY_5;
        case '6': return Keyboard::KEY_6;
        case '7': return Keyboard::KEY_7;
        case '8': return Keyboard::KEY_8;
        case '9': return Keyboard::KEY_9;

        // Letters
        case 'A': return Keyboard::KEY_A;
        case 'B': return Keyboard::KEY_B;
        case 'C': return Keyboard::KEY_C;
        case 'D': return Keyboard::KEY_D;
        case 'E': return Keyboard::KEY_E;
        case 'F': return Keyboard::KEY_F;
        case 'G': return Keyboard::KEY_G;
        case 'H': return Keyboard::KEY_H;
        case 'I': return Keyboard::KEY_I;
        case 'J': return Keyboard::KEY_J;
        case 'K': return Keyboard::KEY_K;
        case 'L': return Keyboard::KEY_L;
        case 'M': return Keyboard::KEY_M;
        case 'N': return Keyboard::KEY_N;
        case 'O': return Keyboard::KEY_O;
        case 'P': return Keyboard::KEY_P;
        case 'Q': return Keyboard::KEY_Q;
        case 'R': return Keyboard::KEY_R;
        case 'S': return Keyboard::KEY_S;
        case 'T': return Keyboard::KEY_T;
        case 'U': return Keyboard::KEY_U;
        case 'V': return Keyboard::KEY_V;
        case 'W': return Keyboard::KEY_W;
        case 'X': return Keyboard::KEY_X;
        case 'Y': return Keyboard::KEY_Y;
        case 'Z': return Keyboard::KEY_Z;

        case VK_CONTROL: return extended ? Keyboard::KEY_RIGHT_CTRL : Keyboard::KEY_LEFT_CTRL;
        case VK_MENU: return extended ? Keyboard::KEY_RIGHT_ALT : Keyboard::KEY_LEFT_ALT;
        case VK_LWIN: return Keyboard::KEY_LEFT_SUPER;
        case VK_RWIN: return Keyboard::KEY_RIGHT_SUPER;

        // Navigation / editing
        case VK_TAB: return Keyboard::KEY_TAB;
        case VK_CAPITAL: return Keyboard::KEY_CAPS_LOCK;
        case VK_SPACE: return Keyboard::KEY_SPACE;
        case VK_RETURN: return extended ? Keyboard::KEY_NUMPAD_ENTER : Keyboard::KEY_ENTER;
        case VK_BACK: return Keyboard::KEY_BACKSPACE;
        case VK_ESCAPE: return Keyboard::KEY_ESCAPE;
        case VK_INSERT: return Keyboard::KEY_INSERT;
        case VK_DELETE: return Keyboard::KEY_DELETE;
        case VK_HOME: return Keyboard::KEY_HOME;
        case VK_END: return Keyboard::KEY_END;
        case VK_PRIOR: return Keyboard::KEY_PAGE_UP;
        case VK_NEXT: return Keyboard::KEY_PAGE_DOWN;
        case VK_UP: return Keyboard::KEY_UP;
        case VK_DOWN: return Keyboard::KEY_DOWN;
        case VK_LEFT: return Keyboard::KEY_LEFT;
        case VK_RIGHT: return Keyboard::KEY_RIGHT;

        // Punctuation / symbols
        case VK_OEM_MINUS: return Keyboard::KEY_MINUS;
        case VK_OEM_PLUS: return Keyboard::KEY_EQUALS;
        case VK_OEM_4: return Keyboard::KEY_LEFT_BRACKET;
        case VK_OEM_6: return Keyboard::KEY_RIGHT_BRACKET;
        case VK_OEM_5: return Keyboard::KEY_BACKSLASH;
        case VK_OEM_COMMA: return Keyboard::KEY_COMMA;
        case VK_OEM_PERIOD: return Keyboard::KEY_PERIOD;
        case VK_OEM_1: return Keyboard::KEY_SEMICOLON;
        case VK_OEM_7: return Keyboard::KEY_APOSTROPHE;
        case VK_OEM_3: return Keyboard::KEY_GRAVE;
        case VK_OEM_2: return Keyboard::KEY_FORWARD_SLASH;

        // Function keys
        case VK_F1: return Keyboard::KEY_F1;
        case VK_F2: return Keyboard::KEY_F2;
        case VK_F3: return Keyboard::KEY_F3;
        case VK_F4: return Keyboard::KEY_F4;
        case VK_F5: return Keyboard::KEY_F5;
        case VK_F6: return Keyboard::KEY_F6;
        case VK_F7: return Keyboard::KEY_F7;
        case VK_F8: return Keyboard::KEY_F8;
        case VK_F9: return Keyboard::KEY_F9;
        case VK_F10: return Keyboard::KEY_F10;
        case VK_F11: return Keyboard::KEY_F11;
        case VK_F12: return Keyboard::KEY_F12;
        case VK_F13: return Keyboard::KEY_F13;
        case VK_F14: return Keyboard::KEY_F14;
        case VK_F15: return Keyboard::KEY_F15;

        // Numpad
        case VK_NUMPAD0: return Keyboard::KEY_NUMPAD_0;
        case VK_NUMPAD1: return Keyboard::KEY_NUMPAD_1;
        case VK_NUMPAD2: return Keyboard::KEY_NUMPAD_2;
        case VK_NUMPAD3: return Keyboard::KEY_NUMPAD_3;
        case VK_NUMPAD4: return Keyboard::KEY_NUMPAD_4;
        case VK_NUMPAD5: return Keyboard::KEY_NUMPAD_5;
        case VK_NUMPAD6: return Keyboard::KEY_NUMPAD_6;
        case VK_NUMPAD7: return Keyboard::KEY_NUMPAD_7;
        case VK_NUMPAD8: return Keyboard::KEY_NUMPAD_8;
        case VK_NUMPAD9: return Keyboard::KEY_NUMPAD_9;
        case VK_DECIMAL: return Keyboard::KEY_NUMPAD_DECIMAL;
        case VK_ADD: return Keyboard::KEY_NUMPAD_PLUS;
        case VK_SUBTRACT: return Keyboard::KEY_NUMPAD_MINUS;
        case VK_MULTIPLY: return Keyboard::KEY_NUMPAD_MULTIPLY;
        case VK_DIVIDE: return Keyboard::KEY_NUMPAD_DIVIDE;

        // System / special
        case VK_SNAPSHOT: return Keyboard::KEY_PRINT_SCREEN;
        case VK_SCROLL: return Keyboard::KEY_SCROLL_LOCK;
        case VK_PAUSE: return Keyboard::KEY_PAUSE;
        case VK_APPS: return Keyboard::KEY_MENU;

        default: return Keyboard::KEY_COUNT; // invalid
    }
}


void Win64_UpdateKeyState(Keyboard key, bool is_down, KeyboardState* p_kb) 
{
	size_t key_idx = static_cast<size_t>(key);
	p_kb->down[key_idx] = !p_kb->pressed[key_idx] && is_down;
	p_kb->up[key_idx] = p_kb->pressed[key_idx] && !is_down;
	p_kb->pressed[key_idx] = is_down;
}
