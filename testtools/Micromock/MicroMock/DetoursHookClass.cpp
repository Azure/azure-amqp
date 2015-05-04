//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include "stdafx.h"
#include "DetoursHookClass.h"
#include "MicroMockTestRunnerHooks.h"
#include "MicroMockException.h"

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
