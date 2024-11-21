-- Copyright (c) 2020-2024 Jeffery Myers
--
--This software is provided "as-is", without any express or implied warranty. In no event 
--will the authors be held liable for any damages arising from the use of this software.

--Permission is granted to anyone to use this software for any purpose, including commercial 
--applications, and to alter it and redistribute it freely, subject to the following restrictions:

--  1. The origin of this software must not be misrepresented; you must not claim that you 
--  wrote the original software. If you use this software in a product, an acknowledgment 
--  in the product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented
--  as being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.

baseName = path.getbasename(os.getcwd());

workspace (path.getbasename(os.getcwd()))
    location "../"
    configurations { "Debug", "Release" }
    platforms { "x64", "x86", "ARM64"}

    defaultplatform ("x64")

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "platforms:x64" }
        architecture "x86_64"

    filter { "platforms:Arm64" }
        architecture "ARM64"

    filter {}

    targetdir "./%{cfg.buildcfg}/"

    startproject(path.getbasename(os.getcwd()))

    project (path.getbasename(os.getcwd()))
        kind "ConsoleApp"
        location "./"
        targetdir "./%{cfg.buildcfg}"


        filter "action:vs*"
            debugdir "$(SolutionDir)"
  
        flags { "ShadowedVariables"}
        filter{}

		filter "system:windows"
			defines{"_WIN32"}
			links {"winmm", "gdi32"}
			libdirs {"../bin/%{cfg.buildcfg}"}

		filter "system:linux"
			links {"pthread", "m", "dl", "rt"}

		filter "system:macosx"
            links {"Cocoa.framework", "IOKit.framework", "CoreFoundation.framework", "CoreAudio.framework", "CoreVideo.framework", "AudioToolbox.framework"}

		

project (path.getbasename(os.getcwd()))
    kind "ConsoleApp"
    location "./"
    targetdir "./bin/%{cfg.buildcfg}"

    vpaths 
    {
        ["Header Files/*"] = { "include/**.h", "include/**.hpp", "**.h", "**.hpp"},
        ["Source Files/*"] = { "src/**.cpp", "src/**.c", "**.cpp","**.c"},
    }
    files {"**.hpp", "**.h", "**.cpp","**.c"}
	removefiles 
	{ 
		"./sfotool/**"
    }
