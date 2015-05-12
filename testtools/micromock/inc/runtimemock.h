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

#ifndef RUNTIMEMOCK_H
#define RUNTIMEMOCK_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef _MSC_VER
#ifndef DISABLE_DETOURS

#include "stdafx.h"
#include "threadsafeglobalmock.h"
#include "detourshookclass.h"
#include "mockcallcomparer.h"
#include "strictunorderedcallcomparer.h"

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
