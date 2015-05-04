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

#ifndef MOCKMETHODCALLBASE_H
#define MOCKMETHODCALLBASE_H

#pragma once

#include "stdafx.h"
#include "MockValueBase.h"
#include "MockCallArgumentBase.h"

class CMockMethodCallBase
{
public:
    CMockMethodCallBase();
    virtual ~CMockMethodCallBase();
    CMockMethodCallBase(std::tstring methodName, size_t argCount = 0,
        CMockCallArgumentBase** arguments = NULL);

    std::tstring ToString();
    std::tstring GetArgumentsString();
    bool operator==(const CMockMethodCallBase& right);
    void CopyOutArgumentBuffers(CMockMethodCallBase* sourceMockMethodCall);
    void AddExtraCallArgument(CMockCallArgumentBase* callArgument);

    CMockMethodCallBase* m_MatchedCall;
    bool m_OnlySpecifiesActions;
    bool m_AlwaysReport;
    size_t m_ExpectedTimes;
    size_t m_MatchedTimes;
    bool m_ExactExpectedTimes;
    virtual CMockValueBase* GetReturnValue() { return m_ReturnValue; }
    _Must_inspect_result_ bool HasMatch() const { return (NULL != m_MatchedCall); }
    void RollbackMatch();

protected:
    void Init(std::tstring methodName);

    std::vector<CMockCallArgumentBase*> m_MockCallArguments;
    std::tstring m_MethodName;
    CMockValueBase* m_ReturnValue;
};

#endif // MOCKMETHODCALLBASE_H
