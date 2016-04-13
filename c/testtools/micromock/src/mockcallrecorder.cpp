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
#include "mockcallrecorder.h"
#include "mockvaluebase.h"
#include "micromocktestrunnerhooks.h"
#include "mockcallcomparer.h"
#include "micromockexception.h"

CMockCallRecorder::CMockCallRecorder(AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison, CMockCallComparer* mockCallComparer) :
    m_PerformAutomaticCallComparison(performAutomaticCallComparison),
    m_MockCallComparer(mockCallComparer)
{
    MicroMockInitializeCriticalSection(&m_MockCallRecorderCS);
}

CMockCallRecorder::~CMockCallRecorder(void)
{
    if (m_PerformAutomaticCallComparison)
    {
        AssertActualAndExpectedCalls();
    }

    ResetAllCalls();

    MicroMockDeleteCriticalSection(&m_MockCallRecorderCS);
}

void CMockCallRecorder::AssertActualAndExpectedCalls(void)
{
#if defined CPP_UNITTEST || defined USE_CTEST
    if ((GetMissingCalls().length()>0) || (GetUnexpectedCalls().length()>0))
    {
        std::tstring result = std::tstring(_T("Expected: ")) + GetMissingCalls() + std::tstring(_T(" Actual: ")) + GetUnexpectedCalls();
        MOCK_FAIL(result.c_str());
    }
#else /*so it is TAEF*/
    WEX::Common::String missingCalls(GetMissingCalls().c_str());
    WEX::Common::String unexpectedCalls(GetUnexpectedCalls().c_str());
    MOCK_ASSERT(missingCalls, unexpectedCalls, _T("Missing calls do not match expected calls"));
#endif

    m_PerformAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_OFF;
}

void CMockCallRecorder::RecordExpectedCall(CMockMethodCallBase* mockMethodCall)
{
    Lock();
    m_ExpectedCalls.push_back(mockMethodCall);
    Unlock();
}

CMockValueBase* CMockCallRecorder::RecordActualCall(CMockMethodCallBase* mockMethodCall)
{
    CMockValueBase* result = NULL;

    MicroMockEnterCriticalSection(&m_MockCallRecorderCS);

    m_ActualCalls.push_back(mockMethodCall);
    result = MatchActualCall(mockMethodCall);
    MicroMockLeaveCriticalSection(&m_MockCallRecorderCS);

    return result;
}

CMockValueBase* CMockCallRecorder::MatchActualCall(CMockMethodCallBase* mockMethodCall)
{
    CMockValueBase* result = NULL;

    MicroMockEnterCriticalSection(&m_MockCallRecorderCS);

    if (mockMethodCall->HasMatch())
    {
        mockMethodCall->m_MatchedCall->RollbackMatch();
        mockMethodCall->RollbackMatch();
    }

    CMockMethodCallBase* matchedCall = NULL;
    if (NULL != m_MockCallComparer)
    {
        matchedCall = m_MockCallComparer->MatchCall(m_ExpectedCalls, mockMethodCall);
    }
    else
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INTERNAL_ERROR,
            _T("Invalid Mock Call Comparer")));
    }

    if (NULL != matchedCall)
    {
        mockMethodCall->CopyOutArgumentBuffers(matchedCall);
        result = matchedCall->GetReturnValue();
    }

    MicroMockLeaveCriticalSection(&m_MockCallRecorderCS);

    return result;
}

std::tstring CMockCallRecorder::CompareActualAndExpectedCalls()
{
    return GetMissingCalls(_T("Expected:")) + GetUnexpectedCalls(_T("Actual:"));
}

std::tstring CMockCallRecorder::GetUnexpectedCalls(std::tstring unexpectedCallPrefix)
{
    std::tostringstream result;

    if (NULL != m_MockCallComparer)
    {
        for (size_t i = 0; i < m_ActualCalls.size(); i++)
        {
            if (m_MockCallComparer->IsUnexpectedCall(m_ActualCalls[i]))
            {
                result << _T("[");
                result << unexpectedCallPrefix;
                result << m_ActualCalls[i]->ToString();
                result << _T("]");
            }
        }
    }
    else
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INTERNAL_ERROR,
            _T("Invalid Mock Call Comparer")));
    }

    m_PerformAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_OFF;

    return result.str();
}

std::tstring CMockCallRecorder::GetMissingCalls(std::tstring missingCallPrefix)
{
    std::tostringstream result;

    if (NULL != m_MockCallComparer)
    {
        for (size_t i = 0; i < m_ExpectedCalls.size(); i++)
        {
            if (m_MockCallComparer->IsMissingCall(m_ExpectedCalls[i]))
            {
                result << _T("[");
                result << missingCallPrefix;
                result << m_ExpectedCalls[i]->ToString();
                result << _T("]");

                if (m_ExpectedCalls[i]->m_ExpectedTimes - m_ExpectedCalls[i]->m_MatchedTimes > 1)
                {
                    result << _T("(x");
                    result << m_ExpectedCalls[i]->m_ExpectedTimes - m_ExpectedCalls[i]->m_MatchedTimes;
                    result << _T(")");
                }
            }
        }
    }
    else
    {
        MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INTERNAL_ERROR,
            _T("Invalid Mock Call Comparer")));
    }

    m_PerformAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_OFF;

    return result.str();
}

void CMockCallRecorder::ResetExpectedCalls()
{
    MicroMockEnterCriticalSection(&m_MockCallRecorderCS);

    for (size_t i = 0; i < m_ExpectedCalls.size(); i++)
    {
        delete m_ExpectedCalls[i];
    }

    for (size_t i = 0; i < m_ActualCalls.size(); i++)
    {
        m_ActualCalls[i]->m_MatchedCall = NULL;
    }

    m_ExpectedCalls.clear();

    MicroMockLeaveCriticalSection(&m_MockCallRecorderCS);
}

void CMockCallRecorder::ResetActualCalls()
{
    MicroMockEnterCriticalSection(&m_MockCallRecorderCS);

    for (size_t i = 0; i < m_ActualCalls.size(); i++)
    {
        delete m_ActualCalls[i];
    }

    for (size_t i = 0; i < m_ExpectedCalls.size(); i++)
    {
        m_ExpectedCalls[i]->m_MatchedCall = NULL;
    }

    m_ActualCalls.clear();

    MicroMockLeaveCriticalSection(&m_MockCallRecorderCS);
}

void CMockCallRecorder::ResetAllCalls()
{
    ResetActualCalls();
    ResetExpectedCalls();
}

void CMockCallRecorder::SetPerformAutomaticCallComparison(AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison)
{
    m_PerformAutomaticCallComparison = performAutomaticCallComparison;
}

SAL_Acquires_lock_(m_MockCallRecorderCS)
void CMockCallRecorder::Lock()
{
    MicroMockEnterCriticalSection(&m_MockCallRecorderCS);
}

SAL_Releases_lock_(m_MockCallRecorderCS)
void CMockCallRecorder::Unlock()
{
    MicroMockLeaveCriticalSection(&m_MockCallRecorderCS);
}
