// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>

#include <vector>
#include <sstream>
#include <fstream>

#include <shellscalingapi.h>

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>

// Define the appropriate precision ("float" for DirectX, "double" for standard library)
#define SAVERFLOAT float
