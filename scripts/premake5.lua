workspace "VulkanBase"
	basedir "../"
	filename "VulkanBase"
	configurations { "Debug", "Release" }
	platforms {"x86", "x64"}
	characterset ("MBCS")
	language "C++"

	project "VulkanBase"
		filename "VulkanBase"
   		rtti "Off"
		location "../"
		targetdir "../Build/$(ProjectName)_$(Platform)_$(Configuration)"
		objdir "!../Build/Intermediate/$(ProjectName)_$(Platform)_$(Configuration)"
		pchheader "stdafx.h"
		pchsource "../stdafx.cpp"
		kind "ConsoleApp"

		files
		{ 
			"../**.h",
			"../**.hpp",
			"../**.cpp",
			"../**.inl"
		}

		excludes
		{
			"../external/**"
		}

		includedirs 
		{ 
			"../external/SDL2-2.0.7/include",
			"../external/VulkanSDK/1.1.73.0/include",
			"../external/glm",
			"../external/SPIRV-cross/include",
		}

		libdirs
		{
			"../external/SDL2-2.0.7/lib/%{cfg.platform}",
		}

		filter { "platforms:x86" }
			libdirs
			{
				"../external/VulkanSDK/1.1.73.0/Lib32",
			}

		filter { "platforms:x64" }
			libdirs
			{
				"../external/VulkanSDK/1.1.73.0/Lib",
			}

		postbuildcommands
		{ 
			"{COPY} \"$(SolutionDir)external\\SDL2-2.0.7\\lib\\%{cfg.platform}\\SDL2.dll\" \"$(OutDir)\"",
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