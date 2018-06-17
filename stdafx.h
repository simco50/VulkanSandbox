#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#define VULKAN

#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <windows.h>
#endif

#ifdef VULKAN
#include <vulkan/vulkan.h>

#ifdef PLATFORM_WINDOWS
#include <vulkan/vulkan_win32.h>
#endif

#endif

#include <SDL.h>
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