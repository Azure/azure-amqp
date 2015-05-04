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
using namespace std;
TYPED_MOCK_CLASS(CTestAllArgsMock, CMock)
{
public:
    MOCK_METHOD_0(, UINT8, TestFunctionWithNoArgs);
    MOCK_METHOD_END(UINT8, 0)
    
    MOCK_METHOD_1(, UINT8, TestFunctionWith1Arg, UINT8, arg1);
    MOCK_METHOD_END(UINT8, 0)
    
    MOCK_METHOD_2(, UINT8, TestFunctionWith2Args, UINT8, arg1, UINT8, arg2);
    MOCK_METHOD_END(UINT8, 0)
    
    MOCK_METHOD_3(, UINT8, TestFunctionWith3Args, UINT8, arg1, UINT8, arg2, UINT8, arg3);
    MOCK_METHOD_END(UINT8, 0)
    
    MOCK_METHOD_4(, UINT8, TestFunctionWith4Args, UINT8, arg1, UINT8, arg2, UINT8, arg3, UINT8, arg4);
    MOCK_METHOD_END(UINT8, 0)
    
    MOCK_METHOD_5(, UINT8, TestFunctionWith5Args, UINT8, arg1, UINT8, arg2, UINT8, arg3, UINT8, arg4, UINT8, arg5);
    MOCK_METHOD_END(UINT8, 0)

    MOCK_METHOD_6(, UINT8, TestFunctionWith6Args, UINT8, arg1, UINT8, arg2, UINT8, arg3, UINT8, arg4, UINT8, arg5, UINT8, arg6);
    MOCK_METHOD_END(UINT8, 0)
};

       
    BEGIN_TEST_SUITE(MicroMockCallComparisonUnitTests)

        // GetUnexpectedCalls

        TEST_FUNCTION(MicroMock_GetUnexpectedCalls_When_No_Actual_Call_Is_Made_Returns_An_Empty_String)
        {
            // arrange
            CTestAllArgsMock testMock;

            // act

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("")).c_str(), testMock.GetUnexpectedCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetUnexpectedCalls_Returns_Unexpected_Calls)
        {
            // arrange
            CTestAllArgsMock testMock;

            // act
            testMock.TestFunctionWithNoArgs();

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("[TestFunctionWithNoArgs()]")).c_str(), testMock.GetUnexpectedCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetUnexpectedCalls_When_An_Expected_Call_And_An_Actual_Call_Match_Returns_Empty_String)
        {
            // arrange
            CTestAllArgsMock testMock;

            STRICT_EXPECTED_CALL(testMock, TestFunctionWithNoArgs());

            // act
            testMock.TestFunctionWithNoArgs();

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("")).c_str(), testMock.GetUnexpectedCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetUnexpectedCalls_When_Only_The_Expected_Call_Happens_Returns_Empty_String)
        {
            // arrange
            CTestAllArgsMock testMock;

            STRICT_EXPECTED_CALL(testMock, TestFunctionWithNoArgs());

            // act

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("")).c_str(), testMock.GetUnexpectedCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetUnexpectedCalls_When_More_Than_One_Unexpected_Calls_Are_Made_Reports_All_Calls)
        {
            // arrange
            CTestAllArgsMock testMock;

            // act
            testMock.TestFunctionWithNoArgs();
            testMock.TestFunctionWithNoArgs();

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("[TestFunctionWithNoArgs()][TestFunctionWithNoArgs()]")).c_str(), testMock.GetUnexpectedCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        // GetMissingCalls

        TEST_FUNCTION(MicroMock_GetMissingCalls_When_No_Expected_Call_Is_Programmed_Returns_An_Empty_String)
        {
            // arrange
            CTestAllArgsMock testMock;

            // act

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("")).c_str(), testMock.GetMissingCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetMissingCalls_Returns_MissingCalls)
        {
            // arrange
            CTestAllArgsMock testMock;

            STRICT_EXPECTED_CALL(testMock, TestFunctionWithNoArgs());

            // act

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("[TestFunctionWithNoArgs()]")).c_str(), testMock.GetMissingCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetMissingCalls_When_An_Expected_Call_And_An_Actual_Call_Match_Returns_Empty_String)
        {
            // arrange
            CTestAllArgsMock testMock;

            STRICT_EXPECTED_CALL(testMock, TestFunctionWithNoArgs());

            // act
            testMock.TestFunctionWithNoArgs();

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("")).c_str(), testMock.GetMissingCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetMissingCalls_When_Only_The_Actual_Call_Happens_Returns_Empty_String)
        {
            // arrange
            CTestAllArgsMock testMock;

            // act
            testMock.TestFunctionWithNoArgs();

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("")).c_str(), testMock.GetMissingCalls().c_str(),
                _T("Incorrect actual calls"));
        }

        TEST_FUNCTION(MicroMock_GetMissingCalls_When_More_Than_One_Missing_Calls_Exists_Reports_All_Missing_Calls)
        {
            // arrange
            CTestAllArgsMock testMock;

            STRICT_EXPECTED_CALL(testMock, TestFunctionWithNoArgs());
            STRICT_EXPECTED_CALL(testMock, TestFunctionWithNoArgs());

            // act

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, tstring(_T("[TestFunctionWithNoArgs()][TestFunctionWithNoArgs()]")).c_str(), testMock.GetMissingCalls().c_str(),
                _T("Incorrect actual calls"));
        }
        END_TEST_SUITE(MicroMockCallComparisonUnitTests);
