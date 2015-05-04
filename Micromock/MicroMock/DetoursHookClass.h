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

#ifndef DETOURSHOOKCLASS_H
#define DETOURSHOOKCLASS_H

#pragma once

#ifndef DISABLE_DETOURS

#include "stdafx.h"

typedef struct HOOK_FUNCTION_DATA_TAG
{
    const void* originalFunction;
    const void* whoWillBeCalled;
} HOOK_FUNCTION_DATA;

typedef std::map<const void*, HOOK_FUNCTION_DATA> HOOKED_FUNCTIONS_MAP;

class CDetoursHookClass
{
public:
    virtual ~CDetoursHookClass();

    void InstallHook(_In_ const void* functionToHook, _In_ const void* whoWillBeCalled);
    void UninstallHook(_In_ const void* functionToHook, _In_ const void* whoWillBeCalled);
    void UninstallAllHooks();

protected:
    HOOKED_FUNCTIONS_MAP m_HookedFunctionsToOriginals;
};

#endif // DISABLE_DETOURS

#endif // DETOURSHOOKCLASS_H
