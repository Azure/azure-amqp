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

#ifndef MOCKMETHODCALL_H
#define MOCKMETHODCALL_H

#pragma once

#include "MockMethodCallBase.h"
#include "MockResultValue.h"
#include "MicroMockTestRunnerHooks.h"
#include "MockCallArgumentBase.h"
#include "MicroMockException.h"

template<typename returnType>
class CMockMethodCall : public CMockMethodCallBase
{
public:
    CMockMethodCall(_In_ std::tstring methodName, _In_ size_t argCount = 0, _In_reads_(argCount) CMockCallArgumentBase** arguments = NULL) :
        CMockMethodCallBase(methodName, argCount, arguments)
    {
    }

    CMockMethodCall<returnType>& IgnoreAllArguments()
    {
        for (size_t i = 0; i < m_MockCallArguments.size(); i++)
        {
            IgnoreArgument(i + 1);
        }

        return *this;
    }
    CMockMethodCall<returnType>& NeverInvoked()
    {
        return ExpectedTimesExactly(0);
    }

    CMockMethodCall<returnType>& IgnoreArgument(_In_ size_t argumentNo)
    {
        if ((argumentNo > 0) &&
            (argumentNo <= m_MockCallArguments.size()))
        {
            m_MockCallArguments[argumentNo - 1]->SetIgnored(true);
        }
        else
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INVALID_ARGUMENT,
                    _T("Invalid argument number supplied to IgnoreArgument")));
        }

        return *this;
    }

    CMockMethodCall<returnType>& AddExtraCallArgument(_In_ CMockCallArgumentBase* callArgument)
    {
        m_MockCallArguments.push_back(callArgument);
        return *this;
    }

    CMockMethodCall<returnType>& ValidateArgument(_In_ size_t argumentNo)
    {
        if ((argumentNo > 0) &&
            (argumentNo <= m_MockCallArguments.size()))
        {
            m_MockCallArguments[argumentNo - 1]->SetIgnored(false);
        }
        else
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INVALID_ARGUMENT,
                _T("Invalid argument number supplied to ValidateArgument")));
        }
        return *this;
    }

    template<typename injectedReturnType>
    CMockMethodCall<returnType>& SetReturn(_In_opt_ injectedReturnType returnValue)
    {
        if (NULL != m_ReturnValue)
        {
            delete m_ReturnValue;
        }

        m_ReturnValue = new CMockResultValue<returnType>(returnValue);

        return *this;
    }

    CMockMethodCall<returnType>& OnlySpecifiesActions()
    {
        m_OnlySpecifiesActions = true;
        return *this;
    }
    CMockMethodCall<returnType>& ValidateArgumentBuffer(_In_ size_t argumentNo,
        _In_reads_bytes_opt_(bytesToValidate) const void* expectedBuffer, _In_ size_t bytesToValidate, _In_ size_t byteOffset = 0)
    {
        if ((argumentNo > 0) &&
            (argumentNo <= m_MockCallArguments.size()) &&
            (NULL != expectedBuffer) &&
            (bytesToValidate > 0))
        {
            m_MockCallArguments[argumentNo - 1]->AddBufferValidation(expectedBuffer, bytesToValidate, byteOffset);
        }
        else
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INVALID_ARGUMENT,
                _T("Invalid arguments for ValidateArgumentBuffer.")));
        }

        return *this;
    }
    CMockMethodCall<returnType>& CopyOutArgumentBuffer(_In_ size_t argumentNo,
        _In_reads_bytes_opt_(bytesToCopy) const void* injectedBuffer, _In_ size_t bytesToCopy, _In_ size_t byteOffset = 0)
    {
        if ((NULL == injectedBuffer) ||
            (bytesToCopy == 0))
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INVALID_ARGUMENT,
                _T("Invalid arguments for CopyOutArgumentBuffer.")));
        }

        if ((argumentNo > 0) &&
            (argumentNo <= m_MockCallArguments.size()))
        {
            m_MockCallArguments[argumentNo - 1]->AddCopyOutArgumentBuffer(injectedBuffer, bytesToCopy, byteOffset);
        }

        return *this;
    }

    CMockMethodCall<returnType>& ExpectedTimesExactly(size_t expectedTimes)
    {
        m_ExpectedTimes = expectedTimes;
        m_ExactExpectedTimes = true;
        return *this;
    }

    CMockMethodCall<returnType>& ExpectedAtLeastTimes(size_t expectedTimes)
    {
        if (m_ExactExpectedTimes)
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION,
                _T("Tried to use ExpectedTimesExactly(or NeverInvoked) with ExpectedAtLeastTimes. Invalid combination.")));
        }

        m_ExpectedTimes = expectedTimes;
        return *this;
    }
};

#endif // MOCKMETHODCALL_H
