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

#ifndef RUNTIMEMOCK_H
#define RUNTIMEMOCK_H

#pragma once

#ifdef _MSC_VER
#ifndef DISABLE_DETOURS

#include "stdafx.h"
#include "ThreadSafeGlobalMock.h"
#include "DetoursHookClass.h"
#include "MockCallComparer.h"
#include "StrictUnorderedCallComparer.h"

template<class C>
class CRuntimeMock : public CThreadSafeGlobalMock<C>,
    public CDetoursHookClass
{
public:
    CRuntimeMock(_In_ AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON) :
        CThreadSafeGlobalMock<C>(performAutomaticCallComparison)
    {
    }

    virtual ~CRuntimeMock()
    {
    }

    virtual void RegisterFunction(_In_ const void* functionToRegister, _In_ const void* registerFunctionArgument)
    {
        // functionToRegister is the mock class member,
        // registerFunctionArgument should contain the function that should be hooked (detoured)
        InstallHook(registerFunctionArgument, functionToRegister);
    }

    virtual void UnregisterFunction(_In_ const void* functionToUnregister, _In_ const void* unregisterFunctionArgument)
    {
        // functionToRegister is the mock class member,
        // registerFunctionArgument should contain the function that should have been hooked (detoured)
        UninstallHook(unregisterFunctionArgument, functionToUnregister);
    }
};

#endif // DISABLE_DETOURS
#endif // _MSC_VER

#endif // RUNTIMEMOCK_H
