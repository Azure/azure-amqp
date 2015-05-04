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

#ifndef MOCKCALLRECORDER_H
#define MOCKCALLRECORDER_H

#pragma once

#include "stdafx.h"
#include "MockMethodCallBase.h"
#include "MicroMockTestRunnerHooks.h"
#include "MockCallComparer.h"

typedef enum AUTOMATIC_CALL_COMPARISON_TAG
{
    AUTOMATIC_CALL_COMPARISON_OFF,
    AUTOMATIC_CALL_COMPARISON_ON
} AUTOMATIC_CALL_COMPARISON;

class CMockCallRecorder
{
public:
    CMockCallRecorder(AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON,
        CMockCallComparer* mockCallComparer = NULL);
    virtual ~CMockCallRecorder(void);

public:
    void RecordExpectedCall(CMockMethodCallBase* mockMethodCall);
    CMockValueBase* RecordActualCall(CMockMethodCallBase* mockMethodCall);
    CMockValueBase* MatchActualCall(CMockMethodCallBase* mockMethodCall);
    void AssertActualAndExpectedCalls(void);
    std::tstring CompareActualAndExpectedCalls(void);
    std::tstring GetUnexpectedCalls(std::tstring unexpectedCallPrefix = _T(""));
    std::tstring GetMissingCalls(std::tstring missingCallPrefix = _T(""));
    void ResetExpectedCalls();
    void ResetActualCalls();
    void ResetAllCalls();
    SAL_Acquires_lock_(m_MockCallRecorderCS) void Lock();
    SAL_Releases_lock_(m_MockCallRecorderCS) void Unlock();
    void SetPerformAutomaticCallComparison(AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison);
    void SetMockCallComparer(CMockCallComparer* mockCallComparer) { m_MockCallComparer = mockCallComparer; };

protected:
    std::vector<CMockMethodCallBase*> m_ExpectedCalls;
    std::vector<CMockMethodCallBase*> m_ActualCalls;

protected:
    MICROMOCK_CRITICAL_SECTION m_MockCallRecorderCS;
    AUTOMATIC_CALL_COMPARISON m_PerformAutomaticCallComparison;
    CMockCallComparer* m_MockCallComparer;
};

#endif // MOCKCALLRECORDER_H
