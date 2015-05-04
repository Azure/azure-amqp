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

#include "stdafx.h"
#include "MockMethodCallBase.h"

CMockMethodCallBase::CMockMethodCallBase()
{
    Init(_T(""));
}

CMockMethodCallBase::~CMockMethodCallBase()
{
    if (NULL != m_ReturnValue)
    {
        delete m_ReturnValue;
    }

    for (size_t i = 0; i < m_MockCallArguments.size(); i++)
    {
        delete m_MockCallArguments[i];
    }
}

CMockMethodCallBase::CMockMethodCallBase(std::tstring methodName, size_t argCount, CMockCallArgumentBase** arguments)
{
    Init(methodName);

    for (unsigned char i = 0; i < argCount; i++)
    {
        m_MockCallArguments.push_back(arguments[i]);
    }
}

void CMockMethodCallBase::Init(std::tstring methodName)
{
    m_ReturnValue = NULL;
    m_MethodName = methodName;
    m_MatchedCall = NULL;
    m_OnlySpecifiesActions = false;
    m_ExpectedTimes = 1;
    m_MatchedTimes = 0;
    m_AlwaysReport = false;
    m_ExactExpectedTimes = false;
}

std::tstring CMockMethodCallBase::GetArgumentsString()
{
    std::tstring result;

    for (size_t i = 0; i < m_MockCallArguments.size(); i++)
    {
        if (result.length() > 0)
        {
            result += _T(",");
        }

        result += m_MockCallArguments[i]->ToString();
    }

    return result;
}

std::tstring CMockMethodCallBase::ToString()
{
    std::tstring result = m_MethodName;

    result += _T("(");
    result += GetArgumentsString();
    result += _T(")");

    return result;
}

void CMockMethodCallBase::RollbackMatch()
{
    m_MatchedCall = NULL;
}

void CMockMethodCallBase::AddExtraCallArgument(CMockCallArgumentBase* callArgument)
{
    m_MockCallArguments.push_back(callArgument);
}

bool CMockMethodCallBase::operator==(const CMockMethodCallBase& right)
{
    bool result = (m_MethodName == right.m_MethodName);
    result = result && (m_MockCallArguments.size() == right.m_MockCallArguments.size());
    if (result)
    {
        for (size_t i = 0; i < m_MockCallArguments.size(); i++)
        {
            if (!(m_MockCallArguments[i]->EqualTo(right.m_MockCallArguments[i])))
            {
                result = false;
            }
        }
    }

    return result;
}

void CMockMethodCallBase::CopyOutArgumentBuffers(CMockMethodCallBase* sourceMockMethodCall)
{
    if (m_MockCallArguments.size() == sourceMockMethodCall->m_MockCallArguments.size())
    {
        for (size_t i = 0; i < m_MockCallArguments.size(); i++)
        {
            // TODO: This should also be handled when comparing calls ...
            (void)m_MockCallArguments[i]->CopyOutArgumentDataFrom(sourceMockMethodCall->m_MockCallArguments[i]);
        }
    }
}
