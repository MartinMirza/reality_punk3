// reality_punk/src/platform/win64/RPWin64.cpp
#include <memory>
#include <string>
#include <windows.h>


#include "core/common.h"
#include "core/game_interface.h"
#include "core/keyboard.h"
#include "platform/win64/RenderThread.h"
#include "shared_runtime/Renderer.h"

static LONG64 FRAME_INDEX;
static bool IS_RUNNING;
static GameAPI> gameAPI = nullptr;
static HMODULE gameDLL = nullptr;

// Load the game DLL
bool LoadGameDLL(const char* dllPath)
{
    if (gameDLL)
    {
        FreeLibrary(gameDLL);
        gameDLL = nullptr;
        gameAPI = nullptr;
    }

    gameDLL = LoadLibraryA(dllPath);
    if (!gameDLL)
    {
        LOG_CRITICAL("Failed to find GameAPI in DLL\n");
        return false;
    }

    gameAPI = reinterpret_cast<GameAPI*>(GetProcAddress(gameDLL, "Game"));
    if (!gameAPI)
    {
        LOG_CRITICAL("Failed to find GameAPI in DLL\n");
        FreeLibrary(gameDLL);
        gameDLL = nullptr;
        return false;
    }

    return true;
}

// Unload the game DLL
void UnloadGameDLL()
{
    if (gameDLL)
    {
        FreeLibrary(gameDLL);
      
        gameDLL = nullptr;
        gameAPI = nullptr;
    }
}

static u64 GetLastTimeFileWasUpdated(WIN32_FILE_ATTRIBUTE_DATA fileInfo)
{
    ULARGE_INTEGER fileTime;
    fileTime.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    fileTime.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    return fileTime.QuadPart;
}

// Check if a file has been updated (for hot-reloading)
bool IsFileUpdated(const char* filePath, uint64_t* lastWriteTime)
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExA(filePath, GetFileExInfoStandard, &fileInfo))
    {
        return false;
    }

    uint64_t currentWriteTime = GetLastTimeFileWasUpdated(fileInfo);

    if (currentWriteTime != *lastWriteTime)
    {
        *lastWriteTime = currentWriteTime;
        return true;
    }
    return false;
}

static void ProcessPeekedMessage(const MSG& message, KeyboardState& out_kb)
{
	switch (message.message)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			Keyboard key = Win32PollKeyboardKey(message.wParam, message.lParam);
			UpdateKeyState(key, true, out_kb);
			break;
		}

		case WM_KEYUP:
		case WM_SYSKEYUP: {
			Keyboard key = Win32PollKeyboardKey(message.wParam, message.lParam);
			UpdateKeyState(key, false, out_kb);
			break;
		}

		default: {
			TranslateMessage(&message);
            DispatchMessage(&message);
		}
		break;
	}
}


LRESULT CALLBACK Win32MessagesHandler(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_DESTROY:
    case WM_QUIT:
    case WM_CLOSE:
        {
            IS_RUNNING = false;
            return 0;
        }
        break;
    default:
        {
            return DefWindowProc(window, message, w_param, l_param);
        }
        break;
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, PSTR command_line, int show_code)
{
    std::string command_line_string = command_line;
    if (command_line_string == "-waitfordebugger")
    {
        while (!IsDebuggerPresent())
        {
            Sleep(0);
        }    
    }
    
    u8 buff[600];
    MemArena scratch(&buff[0], 600);
    
    GameLoadInfo game_info = Platform::GetGameLoadInfo(scratch);
    
    void* raw_persistent_memory_pool = VirtualAlloc(nullptr, game_info.persistentMemoryPoolBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    MemArena permanent_arena(static_cast<u8*>(raw_persistent_memory_pool), game_info.persistentMemoryPoolBytes);
    
    void* raw_transient_memory_pool = VirtualAlloc(nullptr, game_info.transientMemoryPoolBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    MemArena transient_arena(static_cast<u8*>(raw_transient_memory_pool), game_info.transientMemoryPoolBytes);
    
    RenderBuffers render_buffers {};
    render_buffers.writeIndex = 0;
    render_buffers.readIndex = 0;
    
    void* buffers_memory = VirtualAlloc(nullptr, (game_info.renderCommandMemoryPoolBytes + sizeof(RenderCommandBuffer)) * RENDER_BUFFER_COUNT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    for (i32 i = 0; i < RENDER_BUFFER_COUNT; i++)
    {
        void* raw_render_command_buffer = static_cast<u8*>(buffers_memory) + (i * (sizeof(RenderCommandBuffer) + game_info.renderCommandMemoryPoolBytes));
        render_buffers.commandBuffers[i] = new (raw_render_command_buffer) RenderCommandBuffer((u8*)raw_render_command_buffer + sizeof(RenderCommandBuffer), game_info.renderCommandMemoryPoolBytes);
    }

    GameMemory game_memory {};
    game_memory.persistentStorage = &permanent_arena;
    game_memory.transientStorage = &transient_arena;
    game_memory.renderCommandBuffer = render_buffers.commandBuffers[render_buffers.writeIndex];

    RP2String vanilla_file_name { RP2Str::Build(scratch)
    .Append(game_info.path.c_str())
    .Append(game_info.name.c_str())
    .Append(".dll")
    .Get() };
    
    RP2String live_file_name { RP2Str::Build(scratch)
    .Append(game_info.path.c_str())
    .Append(game_info.name.c_str())
    .Append("_live.dll")
    .Get() };  
    
    auto hash_func = [](const RP2String& input)
    {
        u32 hash = 0;

        char* s = input.buffer;
        while (*s) {
            hash ^= (unsigned char)(*s);
            s++;
        }
        
        return hash;
        
        for(char* s = input.buffer; *s; ++s)
        {
            hash += *s;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }

        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);

        return hash;
    };
    
    auto eq_func = [](const RP2String& str1, const RP2String& str2)
    {
        return strcmp(str1.buffer, str2.buffer) == 0;
    };
    
    gameAPI->GameInit(game_memory, settings);
    
    WNDCLASSEX window_class = {};
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW ;
    window_class.lpfnWndProc = Win32MessagesHandler;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"RPWindowClass";
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.cbSize = sizeof(WNDCLASSEX);

    if (RegisterClassEx(&window_class))
    {
        RECT window_rect = {0, 0, static_cast<LONG>(settings.width), static_cast<LONG>(settings.height)};
        HWND window = CreateWindowEx(0, window_class.lpszClassName,
                                     StringToWString(game_info.name).c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     window_rect.right - window_rect.left,
                                     window_rect.bottom - window_rect.top,
                                     nullptr, nullptr, instance, nullptr);

        AbstractRenderer renderer;
        
        CreationArgs renderer_args { window, settings };
        renderer.CreateRenderer(renderer_args);
        
        RenderThreadContext rt_context {};
        rt_context.renderer = &renderer;
        rt_context.buffers = &render_buffers;
        rt_context.semaphore = CreateSemaphoreA(nullptr, 0, 1, nullptr);
        rt_context.renderSemaphore = CreateSemaphoreA(nullptr, 1, 1, nullptr);
        
        HANDLE render_thread_handle = CreateThread(nullptr, 0, &RunRenderThread, &rt_context, 0, nullptr);

        LARGE_INTEGER cyclesAtFrameStart = {};
        LARGE_INTEGER cyclesPerSecond = {};
        LARGE_INTEGER cyclesCachedFromLastFrame = {};
        
        IS_RUNNING = TRUE;
        WIN32_FILE_ATTRIBUTE_DATA file_info;
        if (!GetFileAttributesExA(vanilla_file_name.buffer, GetFileExInfoStandard, &file_info))
        {
            LOG_CRITICAL("Failed to get file attributes for %s", vanilla_file_name);
            return 1;
        }

        QueryPerformanceFrequency(&cyclesPerSecond);
        QueryPerformanceCounter(&cyclesCachedFromLastFrame);
        
        u64 last_dll_write_time = GetLastTimeFileWasUpdated(file_info);
        HANDLE gpu_fence_event = CreateEventA(nullptr, false, 0, nullptr);
        
        KeyboardState keyboard_state {};
        
        rt_context.isRunning = true;

        i64 counter = 0;
        float seconds = 0;
        while (IS_RUNNING)
        {
            counter++;
            MSG message;

            QueryPerformanceCounter(&cyclesAtFrameStart);
            const LONGLONG cycles_elapsed = cyclesAtFrameStart.QuadPart - cyclesCachedFromLastFrame.QuadPart;
            float seconds_elapsed = static_cast<FLOAT>(cycles_elapsed) / static_cast<FLOAT>(cyclesPerSecond.QuadPart);
            cyclesCachedFromLastFrame = cyclesAtFrameStart;
            
            seconds += seconds_elapsed;
            
            if (counter == 1000)
            {
                volatile float mean = seconds / 1000.0f;
                LOG_CRITICAL("Mean: %f", mean);
               // break;
            }
            
           //LOG_INFO("Seconds elapsed: %f", seconds_elapsed);

            memset(keyboard_state.up, false, sizeof(bool) * (INT)Keyboard::KEY_COUNT);
            memset(keyboard_state.down, false, sizeof(bool) * (INT)Keyboard::KEY_COUNT);

            while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                ProcessPeekedMessage(message, keyboard_state);
            }
            
            LONG next_write = (render_buffers.writeIndex + 1) % RENDER_BUFFER_COUNT;
            while (next_write == render_buffers.readIndex)
            {
                WaitForSingleObject(rt_context.renderSemaphore, INFINITE);
            }
            
            game_memory.renderCommandBuffer = render_buffers.commandBuffers[render_buffers.writeIndex];
            game_memory.renderCommandBuffer->Reset();

            if (game_memory.requestReinit)
            {
                //TODO: Resample renderer when I add resizing
                game_memory.persistentStorage->RewindToStart(true);
                gameAPI->GameInit(game_memory, settings);
                game_memory.requestReinit = false;
            }

            if (gameAPI && gameAPI->UpdateAndRender)
            {
                //TODO: clamp is hacky, but I don't want to deal with this now
                gameAPI->UpdateAndRender(game_memory, settings, keyboard_state, HMM_Clamp(seconds_elapsed, 0.0f, 0.1f));
            }
            
            render_buffers.writeIndex = next_write;
            game_memory.renderCommandBuffer->frameIndex = ++FRAME_INDEX;
            
            ReleaseSemaphore(rt_context.semaphore, 1, nullptr);
            
            transient_arena.RewindToStart(true);
            
            if (IsFileUpdated(vanilla_file_name.buffer, &last_dll_write_time))
            {
                LOG_INFO("Game DLL updated. Reloading...");

                UnloadGameDLL();
                Sleep(100);
                
                if (!CopyFileA(vanilla_file_name.buffer, live_file_name.buffer, FALSE))
                {
                    LOG_CRITICAL("Failed to copy DLL %d", GetLastError());
                    return false;
                }
                
                if (!LoadGameDLL(live_file_name.buffer))
                {
                    break;
                }
            }
        }
        
        rt_context.isRunning = false;
        ReleaseSemaphore(rt_context.semaphore, 1, nullptr);
        WaitForSingleObject(render_thread_handle, INFINITE);
    }
    else 
    {
        OutputDebugStringA("Window not registered properly\n"); 
    }

    UnloadGameDLL();
    VirtualFree(raw_persistent_memory_pool, 0, MEM_RELEASE);
    VirtualFree(raw_transient_memory_pool, 0, MEM_RELEASE);
    
    return 0;
}
