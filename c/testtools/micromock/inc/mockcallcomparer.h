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

#ifndef MOCKCALLCOMPARER_H
#define MOCKCALLCOMPARER_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "mockmethodcallbase.h"

class CMockCallComparer
{
public:
    CMockCallComparer() :
        m_IgnoreUnexpectedCalls(false)
    {
    }

    void SetIgnoreUnexpectedCalls(_In_ bool ignoreUnexpectedCalls) { m_IgnoreUnexpectedCalls = ignoreUnexpectedCalls; }
    _Must_inspect_result_ bool GetIgnoreUnexpectedCalls(void) { return m_IgnoreUnexpectedCalls; }

    virtual _Must_inspect_result_ bool IsUnexpectedCall(_In_ const CMockMethodCallBase* actualCall) = 0;
    virtual _Must_inspect_result_ bool IsMissingCall(_In_ const CMockMethodCallBase* actualCall) = 0;
    virtual _Must_inspect_result_ CMockMethodCallBase* MatchCall(std::vector<CMockMethodCallBase*>& expectedCalls,
        CMockMethodCallBase* actualCall) = 0;

protected:
    bool m_IgnoreUnexpectedCalls;
};

#endif // MOCKCALLCOMPARER_H
