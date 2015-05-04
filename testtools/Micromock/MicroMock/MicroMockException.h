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

#ifndef MICROMOCKEXCEPTION_H
#define MICROMOCKEXCEPTION_H

#pragma once

#include "stdafx.h"



typedef enum MICROMOCK_EXCEPTION_TAG
{
    MICROMOCK_EXCEPTION_INVALID_VALIDATE_BUFFERS,
    MICROMOCK_EXCEPTION_ALLOCATION_FAILURE,
    MICROMOCK_EXCEPTION_INVALID_ARGUMENT,
    MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION,
    MICROMOCK_EXCEPTION_MOCK_NOT_FOUND,
    MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL,
    MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL,
    MICROMOCK_EXCEPTION_INTERNAL_ERROR,
    MICROMOCK_EXCEPTION_DETOUR_FAILED
} MICROMOCK_EXCEPTION;

extern const TCHAR* MicroMockExceptionToString(_In_ MICROMOCK_EXCEPTION exceptionCode);

class CMicroMockException
{
public:
    CMicroMockException(_In_ MICROMOCK_EXCEPTION exceptionCode, _In_ std::tstring exceptionText) :
        m_ExceptionCode(exceptionCode),
        m_ExceptionString(exceptionText)
    {
    }
    ~CMicroMockException()
    {
    }

    MICROMOCK_EXCEPTION GetMicroMockExceptionCode() const { return m_ExceptionCode; }
    std::tstring GetExceptionString() const { return m_ExceptionString; }

protected:
    MICROMOCK_EXCEPTION m_ExceptionCode;
    std::tstring m_ExceptionString;
};

#ifdef MOCK_TOSTRING
MOCK_TOSTRING<MICROMOCK_EXCEPTION>(const MICROMOCK_EXCEPTION& t) { return  MicroMockExceptionToString(t);}
#endif

#endif // MICROMOCKEXCEPTION_H
