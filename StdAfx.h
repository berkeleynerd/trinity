#ifndef TrinityAL_StdAfx_H
#define TrinityAL_StdAfx_H

#ifdef _MSC_VER
#pragma warning(push, 3)
#endif

#ifdef _WIN32
#define NOMINMAX //don't want that evil microsoft macro
#include <windows.h> 
#endif

#include <string>
#include <cmath>
#include <cstdlib>
#include <cstdint>

#ifdef _WIN32
typedef HWND Tr2WindowHandle;
#else
typedef uintptr_t Tr2WindowHandle;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Tr2Hal.h"

#endif