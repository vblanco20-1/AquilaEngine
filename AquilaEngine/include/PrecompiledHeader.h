#pragma once

// Link library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")


#include "taskflow/taskflow.hpp"


// System includes
#include <windows.h>



// STL includes
#include <iostream>
#include <string>
#include <algorithm>
#include <execution>
#include <random>
#include <map>
#include <array>
#include <vector>
#include <thread>
#include <future>
#include <concurrentqueue.h>
#include <fstream>




//#define SPP_USE_SPP_ALLOC 1
#include <sparsepp/spp.h>

#define IMGUI_API
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"

#undef min
#undef max
#include "entt/entt.hpp"


#include "Remotery.h"

// DirectX includes
#define DIRECTINPUT_VERSION 0x0800
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <dinput.h>


// Safely release a COM object.
template<typename T>
inline void SafeRelease(T& ptr)
{
	if (ptr != NULL)
	{
		ptr->Release();
		ptr = NULL;
	}
}
inline DWORD GetCurrentProcessorNumber() {
	int CPUInfo[4];
	__cpuid(CPUInfo, 1);
	// CPUInfo[1] is EBX, bits 24-31 are APIC ID
	if ((CPUInfo[3] & (1 << 9)) == 0) return -1;  // no APIC on chip
	return (unsigned)CPUInfo[1] >> 24;

}