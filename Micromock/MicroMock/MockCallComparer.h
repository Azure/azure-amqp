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

#ifndef MOCKCALLCOMPARER_H
#define MOCKCALLCOMPARER_H

#pragma once

#include "stdafx.h"
#include "MockMethodCallBase.h"

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
