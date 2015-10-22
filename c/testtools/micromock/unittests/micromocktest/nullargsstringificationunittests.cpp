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

#include "stdafx.h"

using namespace std;
//------------------------------------------------------------------------------------------------------------------------------
// Tests TOC
//------------------------------------------------------------------------------------------------------------------------------
// - Stringify NULL arguments
//        TEST_FUNCTION(MicroMock_TFS102616_NULL_Const_wchar_Pointer_Does_Not_Trigger_Exceptions)
//        TEST_FUNCTION(MicroMock_TFS102616_NULL_Const_char_Pointer_Does_Not_Trigger_Exceptions)
//        TEST_FUNCTION(MicroMock_TFS102616_NULL_wchar_Pointer_Does_Not_Trigger_Exceptions)
//        TEST_FUNCTION(MicroMock_TFS102616_NULL_char_Pointer_Does_Not_Trigger_Exceptions)
// - Stringify non-NULL arguments
//        TEST_FUNCTION(MicroMock_TFS102616_Const_wchar_String_Is_Printed_As_Such_In_Unexpected_Call)
//        TEST_FUNCTION(MicroMock_TFS102616_Const_char_String_Is_Printed_As_Such_In_Unexpected_Call)
//        TEST_FUNCTION(MicroMock_TFS102616_wchar_String_Is_Printed_As_Such_In_Unexpected_Call)
//        TEST_FUNCTION(MicroMock_TFS102616_char_String_Is_Printed_As_Such_In_Unexpected_Call)
//------------------------------------------------------------------------------------------------------------------------------


TYPED_MOCK_CLASS(CSomeMocks, CGlobalMock)
{
public:
    MOCK_STATIC_METHOD_1(,void, XAA_wchar_const, const wchar_t*, p)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, void, XAA_char_const, const char*, p)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, void, XAA_wchar, wchar_t*, p)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, void, XAA_char, char*, p)
    MOCK_VOID_METHOD_END()
};
DECLARE_GLOBAL_MOCK_METHOD_1(CSomeMocks, , void, XAA_wchar_const, const wchar_t*, p)
DECLARE_GLOBAL_MOCK_METHOD_1(CSomeMocks, , void, XAA_char_const, const char*, p)
DECLARE_GLOBAL_MOCK_METHOD_1(CSomeMocks, , void, XAA_wchar, wchar_t*, p)
DECLARE_GLOBAL_MOCK_METHOD_1(CSomeMocks, , void, XAA_char, char*, p)

template<>
bool operator==<char*>(const CMockValue<char*>& lhs, const CMockValue<char*>& rhs)
{
    bool result = false;
    if (lhs.GetValue() == rhs.GetValue())
    {
        result = true;
    } else
    {
        if ((NULL != lhs.GetValue()) &&
            (NULL != rhs.GetValue()) &&
            (strcmp(lhs.GetValue(), rhs.GetValue()) == 0))
        {
            result = true;
        }
    }
    return result;
}

static int tstring_Compare(const std::tstring& a, const std::tstring&b)
{
    return strcmp(a.c_str(), b.c_str());
}
static void tstring_ToString(char* string, size_t bufferSize, std::tstring val)
{
    size_t val_size = strlen(val.c_str());
    strncpy(string, val.c_str(), (val_size >= bufferSize) ? (bufferSize - 1) : val_size);
}

    BEGIN_TEST_SUITE(NULLArgsStringificationTests)

        /*http://webvstf:8080/tfs/web/wi.aspx?pcguid=8947f9e3-3622-497e-ab87-a27e01082a6c&id=102616*/
        TEST_FUNCTION(MicroMock_TFS102616_NULL_Const_wchar_Pointer_Does_Not_Trigger_Exceptions)
        {
            // arrange
            CSomeMocks testMock;

            STRICT_EXPECTED_CALL(testMock, XAA_wchar_const(NULL))
                .NeverInvoked();

            // act
            XAA_wchar_const(NULL);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_wchar_const(NULL)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_NULL_Const_char_Pointer_Does_Not_Trigger_Exceptions)
        {
            // arrange
            CSomeMocks testMock;

            STRICT_EXPECTED_CALL(testMock, XAA_char_const(NULL))
                .NeverInvoked();

            // act
            XAA_char_const(NULL);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_char_const(NULL)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_NULL_wchar_Pointer_Does_Not_Trigger_Exceptions)
        {
            // arrange
            CSomeMocks testMock;

            STRICT_EXPECTED_CALL(testMock, XAA_wchar(NULL))
                .NeverInvoked();

            // act
            XAA_wchar(NULL);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_wchar(NULL)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_NULL_char_Pointer_Does_Not_Trigger_Exceptions)
        {
            // arrange
            CSomeMocks testMock;

            STRICT_EXPECTED_CALL(testMock, XAA_char(NULL))
                .NeverInvoked();

            // act
            XAA_char(NULL);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_char(NULL)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_Const_wchar_String_Is_Printed_As_Such_In_Unexpected_Call)
        {
            // arrange
            CSomeMocks testMock;
            wchar_t val1[] = L"oink";
            wchar_t val2[] = L"oink";

            STRICT_EXPECTED_CALL(testMock, XAA_wchar_const(val1))
                .NeverInvoked();

            // act
            XAA_wchar_const(val2);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_wchar_const(oink)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_Const_char_String_Is_Printed_As_Such_In_Unexpected_Call)
        {
            // arrange
            CSomeMocks testMock;
            char val1[] = "oink";
            char val2[] = "oink";

            STRICT_EXPECTED_CALL(testMock, XAA_char_const(val1))
                .NeverInvoked();

            // act
            XAA_char_const(val2);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_char_const(oink)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_wchar_String_Is_Printed_As_Such_In_Unexpected_Call)
        {
            // arrange
            CSomeMocks testMock;
            wchar_t val1[] = L"oink";
            wchar_t val2[] = L"oink";

            STRICT_EXPECTED_CALL(testMock, XAA_wchar(val1))
                .NeverInvoked();

            // act
            XAA_wchar(val2);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_wchar(oink)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }

        TEST_FUNCTION(MicroMock_TFS102616_char_String_Is_Printed_As_Such_In_Unexpected_Call)
        {
            // arrange
            CSomeMocks testMock;
            char val1[] = "oink";
            char val2[] = "oink";

            STRICT_EXPECTED_CALL(testMock, XAA_char(val1))
                .NeverInvoked();

            // act
            XAA_char(val2);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(tstring, tstring(_T("[Actual:XAA_char(oink)]")), testMock.CompareActualAndExpectedCalls(),
                _T("An unexpected call with NULL argument should be reported"));
        }
        END_TEST_SUITE(NULLArgsStringificationTests)

