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

#ifndef MOCK_H
#define MOCK_H

#pragma once

#include "MockCallRecorder.h"
#include "StrictUnorderedCallComparer.h"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(param)   param = param;
#endif

template<class C>
class CMock : public CMockCallRecorder
{
public:
    CMock(_In_ AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON)
    {
        SetPerformAutomaticCallComparison(performAutomaticCallComparison);
    }

    virtual void RegisterFunction(_In_ const void* functionToRegister, _In_ const void* registerFunctionArgument)
    {
        UNREFERENCED_PARAMETER(functionToRegister);
        UNREFERENCED_PARAMETER(registerFunctionArgument);
    }

    virtual void UnregisterFunction(_In_ const void* functionToUnregister, _In_ const void* unregisterFunctionArgument)
    {
        UNREFERENCED_PARAMETER(functionToUnregister);
        UNREFERENCED_PARAMETER(unregisterFunctionArgument);
    }
};

#endif // MOCK_H
