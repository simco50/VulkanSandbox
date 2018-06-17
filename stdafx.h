#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#define VULKAN

#define NOMINMAX
#include <windows.h>

#ifdef VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#pragma comment(lib, "vulkan-1.lib")
#endif

#include <SDL.h>
#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "sdl2main.lib")
#undef main

#include <glm.hpp>
#include <ext.hpp>

using int64 = __int64;
using uint64 = unsigned long long;
using uint32 = unsigned int;
using int32 = int;


#ifdef VULKAN
using GpuObject = uint64;
#elif defined(DIRECTX)
using GpuObject = void*;
#endif