
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

void a(void)
{
}

void b(void)
{
}

#ifndef DISABLE_DETOURS
TD_MOCK_CLASS(MyMocks)
{
public: 
    MOCK_HOOK_TD_METHOD_0(, void, a)
    MOCK_VOID_METHOD_END()

    MOCK_HOOK_TD_METHOD_0(, void, b)
    MOCK_VOID_METHOD_END()
};


static int iCall=0;
static void theTask(void)
{
    if(iCall==1)
    {
        a();
    }    
    if(iCall==10)
    {
        a();
        b();
    }
    iCall++;
    
}
static HANDLE g_hMutex=NULL;

#endif
BEGIN_TEST_SUITE(MicroMockVoidVoidTest)

#ifndef DISABLE_DETOURS
    TEST_SUITE_INITIALIZE(a)
    {
            g_hMutex = CreateMutex(NULL, FALSE, _T("onlyOneTestAtATime5"));
            ASSERT_ARE_NOT_EQUAL(HANDLE, (HANDLE)NULL, g_hMutex);
    }

    TEST_SUITE_CLEANUP(b)
    {
        if (g_hMutex != NULL)
        {
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
        }
    }
     

        #pragma warning(disable: 26135)
    TEST_FUNCTION_INITIALIZE(c)
    {
        DWORD r = WaitForSingleObject(g_hMutex, INFINITE);
        if (r != WAIT_OBJECT_0)
        {
            ASSERT_FAIL(_T("WAitForSingleOPbject failed"));
        }
        iCall = 0;
    }

    TEST_FUNCTION_CLEANUP(d)
    {
        if (ReleaseMutex(g_hMutex) == 0)
        {
            ASSERT_FAIL(_T("ReleaseMutex(hOneByOne)==0"));
        }
    }


        TEST_FUNCTION(DiscreteMicroMock_ThereIsNothingCalledAtTimeZero)
        {
            // arrange
            MyMocks mocks(theTask);

            // act
            mocks.RunUntilTick(0);

            // assert - left to uMTD (test would fail if at time = 0 a() would be called*/
        }

        TEST_FUNCTION(DiscreteMicroMock_AisCalledAtTimeOne)
        {
            // arrange
            MyMocks mocks(theTask);
            STRICT_EXPECTED_CALL_AT(mocks, 1, a());

            // act
            mocks.RunUntilTick(1);

            // assert - left to uMTD - this test together with the test before guarantee that at t=1 precisely a() is called
        }

        TEST_FUNCTION(DiscreteMicroMock_AisNotCalledAtTimeTwo)
        {
            // arrange
            MyMocks mocks(theTask);
            STRICT_EXPECTED_CALL_AT(mocks, 1, a());

            // act
            mocks.RunUntilTick(2);

            // assert - left to uMTD
        }

        TEST_FUNCTION(DiscreteMicroMock_InTheSameTickOrderOfCallsIsDisregarded_1)
        {
            ///arrange
            MyMocks mocks(theTask);
            STRICT_EXPECTED_CALL_AT(mocks, 1, a());
            STRICT_EXPECTED_CALL_AT(mocks, 10, a());
            STRICT_EXPECTED_CALL_AT(mocks, 10, b());

            ///act
            mocks.RunUntilTick(20);

            ///assert - left to uM_TD
        }

        TEST_FUNCTION(DiscreteMicroMock_InTheSameTickOrderOfCallsIsDisregarded_2)
        {
            ///arrange
            MyMocks mocks(theTask);
            STRICT_EXPECTED_CALL_AT(mocks, 1, a());
            STRICT_EXPECTED_CALL_AT(mocks, 10, b());
            STRICT_EXPECTED_CALL_AT(mocks, 10, a());

            ///act
            mocks.RunUntilTick(20);

            ///assert - left to uM_TD
        }
#endif
        END_TEST_SUITE(MicroMockVoidVoidTest)


