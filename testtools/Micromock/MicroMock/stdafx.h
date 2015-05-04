/*
Copyright (c) Microsoft Corporation.  All rights reserved.

Use of this source code is subject to the terms of the Microsoft end-user
license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
If you did not accept the terms of the EULA, you are not authorized to use
this source code. For a copy of the EULA, please see the LICENSE.RTF on your
install media.
*/

#pragma once

#include "targetver.h"

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "tchar.h"
#include "sal.h"
#include "basetsd.h"
#define SAL_Acquires_lock_(...) _Acquires_lock_(__VA_ARGS__)
#define SAL_Releases_lock_(...) _Releases_lock_(__VA_ARGS__)
typedef CRITICAL_SECTION MICROMOCK_CRITICAL_SECTION;
#define MicroMockEnterCriticalSection(...) EnterCriticalSection(__VA_ARGS__)
#define MicroMockLeaveCriticalSection(...) LeaveCriticalSection(__VA_ARGS__)

#if (defined _WIN32_WCE)
#define MicroMockInitializeCriticalSection(...) InitializeCriticalSection(__VA_ARGS__)
#else
#define MicroMockInitializeCriticalSection(...) InitializeCriticalSectionEx(__VA_ARGS__,2,CRITICAL_SECTION_NO_DEBUG_INFO)
#endif

#define MicroMockDeleteCriticalSection(...) DeleteCriticalSection(__VA_ARGS__)
#else
typedef unsigned char UINT8;
typedef char TCHAR;
#define _T(A)   A
typedef int MICROMOCK_CRITICAL_SECTION;
#define MicroMockEnterCriticalSection(...)
#define MicroMockLeaveCriticalSection(...)
#define MicroMockInitializeCriticalSection(...)
#define MicroMockDeleteCriticalSection(...)
#define SAL_Acquires_lock_(...)
#define SAL_Releases_lock_(...)
#endif

#include "string"
#include "sstream"
#include "map"
#include "algorithm"
#include "iomanip"
#include "vector"
#include "set"

#include "sal.h"

#ifdef _MSC_VER
/*'function' : unreferenced local function has been removed*/
#pragma warning( disable: 4505 ) 

/*unreferenced inline function has been removed*/
#pragma warning( disable: 4514)
#endif

#define COUNT_OF(a)     (sizeof(a) / sizeof((a)[0]))

namespace std
{
    typedef std::basic_string<TCHAR> tstring;
    typedef std::basic_ostringstream<TCHAR> tostringstream;
}