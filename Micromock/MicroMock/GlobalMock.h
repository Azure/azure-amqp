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

#ifndef GLOBALMOCK_H
#define GLOBALMOCK_H

#pragma once

#include "stdafx.h"
#include "Mock.h"
#include "MockCallRecorder.h"
#include "MicroMockException.h"

template<class C>
class CGlobalMock : public CMock<C>
{
public:
    CGlobalMock(_In_ AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON) :
        CMock<C>(performAutomaticCallComparison)
    {
        m_GlobalMockInstance = this;
    }

    virtual ~CGlobalMock()
    {
        m_GlobalMockInstance = NULL;
    }

    _Check_return_
    _Post_satisfies_(return != NULL)
    static CGlobalMock<C>* GetSingleton()
    {
        if (NULL == m_GlobalMockInstance)
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INTERNAL_ERROR,
                _T("Error retrieving singleton")));
        }

        return m_GlobalMockInstance;
    }

protected:
    static CGlobalMock<C>* m_GlobalMockInstance;
};

template<class C>
CGlobalMock<C>* CGlobalMock<C>::m_GlobalMockInstance;

#endif // GLOBALMOCK_H
