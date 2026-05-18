-- =========================
-- CONFIG
-- =========================
RUNTIME_NAME = "reality_punk3"
GAME_NAME = "break_this"
CORE_NAME = "rp3_core"

WIN_PLATFORM_DEF = "PLATFORM_WINDOWS"
LINUX_PLATFORM_DEF = "PLATFORM_LINUX"

RT_DIR = RUNTIME_NAME .. "/"

ROOT_DIR = os.getcwd()
ENGINE_DIR = path.join(ROOT_DIR, "reality_punk/engine")
GAME_DIR = path.join(ROOT_DIR, "reality_punk/game")

-- Function to generate your desired DLL template
function generate_game_dll_template(target_dir, filename)
    local filepath = path.join(target_dir, filename)
    if not os.isfile(filepath) then
        print("Generating game DLL template: " .. filepath)
        local file = io.open(filepath, "w")
        file:write([[
#include "core/GameInterface.h"

static void GameInitInternal(GameMemory* memory, GameWindowSettings* window)
{
    // TODO: Initialize your game here
}

static void GameUpdateAndRenderInternal(GameMemory* memory, GameWindowSettings* window, float deltaTime)
{
    // TODO: Update and render your game here
}

// The ONLY exported symbol
extern "C" GAME_EXPORT GameAPI Game = {
    GameInitInternal,
    GameUpdateAndRenderInternal,
};
        ]])
        file:close()
    end
end

-- =========================
-- CUSTOM OPTIONS
-- =========================
newoption {
    trigger = "gfx",
    value = "API",
    description = "Choose the graphics API to use",
    allowed = {
        { "d3d11", "Direct3D 11" },
        { "opengl", "OpenGL" }
    },
    default = "d3d11"
}

-- =========================
-- WORKSPACE
-- =========================
workspace (RUNTIME_NAME)
    configurations { "Debug", "Release" }
    platforms { "x64" }
	location (RUNTIME_NAME)

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter {}
	
-- =========================
-- ENGINE CORE (STATIC LIB)
-- =========================
project (CORE_NAME)
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"
	location (RUNTIME_NAME)

	filter "system:linux"
		defines { LINUX_PLATFORM_DEF }
        buildoptions { "-fPIC" }
		
	filter "system:windows"
		defines { WIN_PLATFORM_DEF }
	
	filter {}
	
    targetdir ("%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}")
    objdir ("%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/obj/engine")

    files {
        RT_DIR .. "engine/include/core/**.h",
		RT_DIR .. "engine/include/core/**.hpp",
		RT_DIR .. "engine/include/external/**.h",
		RT_DIR .. "engine/include/external/**.hpp",
		RT_DIR .. "engine/src/external/**.cpp",
		RT_DIR .. "engine/src/external/**.c",
		RT_DIR .. "engine/src/core/**.cpp",
		RT_DIR .. "engine/src/core/**.c",
    }

    includedirs {
        RT_DIR .. "engine/include"
    }

-- =========================
-- GAME (DLL)
-- =========================
project (GAME_NAME)
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"
	location (RT_DIR .. "game/" .. GAME_NAME .. "/")

	filter "system:linux"
		defines { LINUX_PLATFORM_DEF }
        buildoptions { "-fPIC" }
		
	filter "system:windows"
		defines { WIN_PLATFORM_DEF }
		
    filter {}
	
	    -- Source files (generate template if missing)
    prebuildcommands {
        generate_game_dll_template(RT_DIR .. "game/" .. GAME_NAME .. "/", GAME_NAME .. ".c")
    }
	
	postbuildcommands {
	-- game content
        '{MKDIR} "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/game/' .. GAME_NAME .. '/content"',
		'{MKDIR} "content"',
        '{COPYDIR} "content" "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/game/' .. GAME_NAME .. '/content/"',
	}


    targetdir ("%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/game/" .. GAME_NAME .. "/")
    objdir ("%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/obj/game/" .. GAME_NAME .. "/")

    files {
		RT_DIR .. "game/" .. GAME_NAME .. "/content/**.**",
        RT_DIR .. "game/" .. GAME_NAME .. "/**.h",
        RT_DIR .. "game/" .. GAME_NAME .. "/**.h",
        RT_DIR .. "game/" .. GAME_NAME .. "/**.cpp",
        RT_DIR .. "game/" .. GAME_NAME .. "/**.c",
		RT_DIR .. "game/" .. GAME_NAME .. "/**.json"
    }

    includedirs {
        RT_DIR .. "engine/include"
    }
	
	dependson { CORE_NAME }
	links { CORE_NAME }

-- =========================
-- RUNTIME (EXE)
-- =========================
project (RUNTIME_NAME)
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"

    targetdir ("%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}")
    objdir ("%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/obj/engine")

    files {
		RT_DIR .. "engine/**.json",
		RT_DIR .. "engine/content/**.**",
		RT_DIR .. "engine/include/shared_runtime/**.hpp",
		RT_DIR .. "engine/include/shared_runtime/**.h",
		RT_DIR .. "engine/src/shared_runtime/**.cpp",
    }

    filter "system:windows"
        defines { WIN_PLATFORM_DEF }
		
	filter { "system:windows", "options:gfx=d3d11"}
		defines { "RENDERER_D3D11" }
        files {
            RT_DIR .. "engine/src/platform/win64/**.cpp",
            RT_DIR .. "engine/include/platform/win64/**.h"
        }

    filter "system:linux"
		defines { LINUX_PLATFORM_DEF }
        links { "xcb", "xcb-keysyms", "X11", "GL" }
		
	filter { "system:linux", "options:gfx=opengl" }
		defines { "RENDERER_OPENGL" }
		files {
            RT_DIR .. "engine/src/platform/linux/**.cpp",
            RT_DIR .. "engine/include/platform/linux/**.h"
		}
		
	filter "files:**.hlsl"
		buildaction "None"

	filter "files:**.glsl"
    	buildaction "None"

    filter {}

    includedirs {
        RT_DIR .. "engine/include"
    }

    dependson { CORE_NAME }
	links { CORE_NAME }

    postbuildcommands {
        -- engine content
        '{MKDIR} "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/engine/content"',
        '{MKDIR} "%{wks.location}/engine/content/"',
        '{COPYDIR} "%{wks.location}/engine/content/" "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/engine/content/"',

        '{MKDIR} "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/engine/config"',
        '{MKDIR} "%{wks.location}/engine/config"',
        '{COPYDIR} "%{wks.location}/engine/config" "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}/engine/config/"',
	}

