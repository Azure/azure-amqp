/*
Copyright (c) Microsoft Corporation.  All rights reserved.

Use of this source code is subject to the terms of the Microsoft end-user
license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
If you did not accept the terms of the EULA, you are not authorized to use
this source code. For a copy of the EULA, please see the LICENSE.RTF on your
install media.
*/

/*defines*/
#include "stdafx.h"
#include "MicroMockException.h"
#include "MicroMockTestRunnerHooks.h"
#define _MACROSTR(a) _T(#a)
/*types*/

/*static variables*/
/*static functions*/
/*variable exports*/

const TCHAR* MicroMockExceptionToString(_In_ MICROMOCK_EXCEPTION exceptionCode)
{
    switch (exceptionCode)
    {
        default:
	        MOCK_THROW(_T("Invalid exception code"));
            break;

        case MICROMOCK_EXCEPTION_INVALID_VALIDATE_BUFFERS:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INVALID_VALIDATE_BUFFERS);
        case MICROMOCK_EXCEPTION_ALLOCATION_FAILURE:
	        return _MACROSTR(MICROMOCK_EXCEPTION_ALLOCATION_FAILURE);
        case MICROMOCK_EXCEPTION_INVALID_ARGUMENT:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INVALID_ARGUMENT);
        case MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION);
        case MICROMOCK_EXCEPTION_MOCK_NOT_FOUND:
	        return _MACROSTR(MICROMOCK_EXCEPTION_MOCK_NOT_FOUND);
        case MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL:
	        return _MACROSTR(MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL);
        case MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL:
	        return _MACROSTR(MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL);
        case MICROMOCK_EXCEPTION_INTERNAL_ERROR:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INTERNAL_ERROR);
        case MICROMOCK_EXCEPTION_DETOUR_FAILED:
            return _MACROSTR(MICROMOCK_EXCEPTION_DETOUR_FAILED);
    }
};

/*function exports*/

