workspace "VulkanBase"
	basedir "../"
	filename "VulkanBase"
	configurations { "Debug", "Release" }
	platforms {"x86", "x64"}
	characterset ("MBCS")
	language "C++"

	configuration "vs*"
		defines { "PLATFORM_WINDOWS" }

	configuration "linux"
		defines { "PLATFORM_LINUX" }

	project "VulkanBase"
		filename "VulkanBase"
   		rtti "Off"
		location "../"
		targetdir "../Build/$(ProjectName)_$(Platform)_$(Configuration)"
		objdir "!../Build/Intermediate/$(ProjectName)_$(Platform)_$(Configuration)"
		pchheader "stdafx.h"
		pchsource "../Source/stdafx.cpp"
		kind "ConsoleApp"

		files
		{ 
			"../Source/**",
		}

		includedirs 
		{ 
			"$(ProjectDir)/Source",
			"../external/SDL2-2.0.7/include",
			"../external/VulkanSDK/1.1.77.0/include",
			"../external/glm",
		}

		libdirs
		{
			"../external/SDL2-2.0.7/lib/%{cfg.platform}",
		}

		links
		{
			"vulkan-1.lib",		
			"sdl2.lib",		
			"sdl2main.lib",		
		}

		postbuildcommands
		{ 
			"{COPY} \"$(SolutionDir)external\\SDL2-2.0.7\\lib\\%{cfg.platform}\\SDL2.dll\" \"$(OutDir)\"",
		}

		filter { "platforms:x86" }
			libdirs
			{
				"../external/VulkanSDK/1.1.77.0/Lib32"
			}

		filter { "platforms:x64" }
			libdirs
			{
				"../external/VulkanSDK/1.1.77.0/Lib"
			}

newaction {
	trigger     = "clean",
	description = "Remove all binaries and generated files",

	execute = function()
		os.rmdir("../Build")
		os.rmdir("../ipch")
		os.remove("../.vs")
		os.remove("../*.sln")
		os.remove("../*.vcxproj.*")
		os.remove("../*.vcxproj.*")
	end
}