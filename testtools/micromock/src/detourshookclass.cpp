/*
Microsoft Azure IoT Device Libraries
Copyright (c) Microsoft Corporation
All rights reserved. 
MIT License
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the Software), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
*/

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "detourshookclass.h"
#include "micromocktestrunnerhooks.h"
#include "micromockexception.h"

#ifndef DISABLE_DETOURS

#include "detours.h" /*this is intentionally here and not in stdafx because users of MicroMock don't want to include in their search path the folder for detours.h*/

CDetoursHookClass::~CDetoursHookClass()
{
    UninstallAllHooks();
}

void CDetoursHookClass::InstallHook(_In_ const void* functionToHook, _In_ const void* whoWillBeCalled)
{
    void* localFunctionToHook = const_cast<void*>(functionToHook);
    HOOK_FUNCTION_DATA hookFunctionData;

    if (NULL == whoWillBeCalled)
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("NULL function to be called used.")));
    }

    if (NULL == functionToHook)
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("NULL function to detour used.")));
    }

    if (m_HookedFunctionsToOriginals.find(functionToHook) != m_HookedFunctionsToOriginals.end())
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Function already hooked")));
    }

    if (DetourTransactionBegin())
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Detour transaction creation failed.")));
    }

    DetourUpdateThread(GetCurrentThread());

    if (DetourAttach(&localFunctionToHook, const_cast<void*>(whoWillBeCalled)))
    {
        DetourTransactionAbort();
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Detour hook attach failed.")));
    }

    if (DetourTransactionCommit())
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Detour transaction commit failed.")));
    }

    hookFunctionData.originalFunction = localFunctionToHook;
    hookFunctionData.whoWillBeCalled = whoWillBeCalled;
    m_HookedFunctionsToOriginals[functionToHook] = hookFunctionData;
}

void CDetoursHookClass::UninstallHook(_In_ const void* functionToHook, _In_ const void* whoWillBeCalled)
{
    HOOKED_FUNCTIONS_MAP::iterator pos = m_HookedFunctionsToOriginals.find(functionToHook);
    if (pos == m_HookedFunctionsToOriginals.end())
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Could not find hooked function.")));
    }

    void* localFunctionToHook = const_cast<void*>(pos->second.originalFunction);

    if (NULL == functionToHook)
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("NULL argument for function to uninstall hook for.")));
    }

    if (NULL == whoWillBeCalled)
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("NULL argument for target hook function.")));
    }

    if (DetourTransactionBegin())
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Detour transaction creation failed.")));
    }

    DetourUpdateThread(GetCurrentThread());

    if (DetourDetach(&localFunctionToHook, const_cast<void*>(whoWillBeCalled)))
    {
        DetourTransactionAbort();
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Detour hook detach failed.")));
    }

    if (DetourTransactionCommit())
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_DETOUR_FAILED, _T("Detour transaction commit failed.")));
    }

    m_HookedFunctionsToOriginals.erase(functionToHook);
}

void CDetoursHookClass::UninstallAllHooks()
{
    while (m_HookedFunctionsToOriginals.size() > 0)
    {
        HOOKED_FUNCTIONS_MAP::iterator pos = m_HookedFunctionsToOriginals.begin();
        const void* functionToDelete = pos->first;
        UninstallHook(functionToDelete, pos->second.whoWillBeCalled);
    }
}

#endif // DISABLE_DETOURS
