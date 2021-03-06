#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <array>

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

#define GLM_ENABLE_EXPERIMENTAL

#include <glm.hpp>
#include <ext.hpp>
#include <gtx/quaternion.hpp>


using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;

using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

#ifdef VULKAN
using GpuObject = uint64;
#elif defined(DIRECTX)
using GpuObject = void*;
#endif

template <typename T>
T Clamp(const T& a, const T& min, const T& max)
{
	if (a < min)
		return min;
	return a > max ? max : a;
}