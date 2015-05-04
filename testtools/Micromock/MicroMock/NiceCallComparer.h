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

#ifndef NICECALLCOMPARER_H
#define NICECALLCOMPARER_H

#pragma once

#include "stdafx.h"
#include "StrictUnorderedCallComparer.h"
#include "MockMethodCallBase.h"

template<class T>
class CNiceCallComparer :
    public CStrictUnorderedCallComparer<T>
{
public:
    CNiceCallComparer(_In_ AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON) :
        CStrictUnorderedCallComparer<T>(performAutomaticCallComparison)
    {
        T::CMockCallRecorder::m_MockCallComparer->SetIgnoreUnexpectedCalls(true);
    }
};

#endif // NICECALLCOMPARER_H
