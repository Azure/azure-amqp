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

BEGIN_TEST_SUITE(CMockValue_tests)

typedef int arrayOf3Int[3];
typedef int arrayOf4Int[4];

/*test a normal array*/
TEST_FUNCTION(CMockValue_TN_toString_1)
{
    arrayOf3Int a = { 1, 2, 3 };
    CMockValue<arrayOf3Int> b(a);

    ASSERT_ARE_EQUAL(char_ptr, "{1,2,3}", b.ToString().c_str());
}

/*test a NULL*/
TEST_FUNCTION(CMockValue_TN_toString_2)
{
    CMockValue<arrayOf3Int> b(NULL);

    ASSERT_ARE_EQUAL(char_ptr, "NULL", b.ToString().c_str());
}

/*test EqualTo from the same source*/
TEST_FUNCTION(CMockValue_TN_EqualTo_1)
{
    arrayOf3Int source = { 1, 2, 3 };
    CMockValue<arrayOf3Int> a(source);
    CMockValue<arrayOf3Int> b(source);

    ASSERT_ARE_EQUAL(int, true, b.EqualTo(&a));
    ASSERT_ARE_EQUAL(int, true, a.EqualTo(&b));
}

/*test EqualTo from different source*/
TEST_FUNCTION(CMockValue_TN_EqualTo_2)
{
    arrayOf3Int source1 = { 1, 2, 3 };
    arrayOf3Int source2 = { 1, 2, 3 };
    CMockValue<arrayOf3Int> a(source1);
    CMockValue<arrayOf3Int> b(source2);

    ASSERT_ARE_EQUAL(int, true, b.EqualTo(&a));
    ASSERT_ARE_EQUAL(int, true, a.EqualTo(&b));
}

/*test EqualTo from different source sizes*/
TEST_FUNCTION(CMockValue_TN_EqualTo_3)
{
    arrayOf4Int source1 = { 1, 2, 3, 4};
    arrayOf3Int source2 = { 1, 2, 3 };
    CMockValue<arrayOf4Int> a3(source1);
    CMockValue<arrayOf3Int> b4(source2);

    ASSERT_ARE_EQUAL(int, false, b4.EqualTo(&a3));
    ASSERT_ARE_EQUAL(int, false, a3.EqualTo(&b4));
}

/*test EqualTo from NULL and NULL*/
TEST_FUNCTION(CMockValue_TN_EqualTo_4)
{
    CMockValue<arrayOf3Int> a(NULL);
    CMockValue<arrayOf3Int> b(NULL);

    ASSERT_ARE_EQUAL(int, true, b.EqualTo(&a));
    ASSERT_ARE_EQUAL(int, true, a.EqualTo(&b));
}

/*test EqualTo from NULL and non-NULL*/
TEST_FUNCTION(CMockValue_TN_EqualTo_5)
{
    arrayOf3Int source = { 1, 2, 3 };
    CMockValue<arrayOf3Int> a(source);
    CMockValue<arrayOf3Int> b(NULL);

    ASSERT_ARE_EQUAL(int, false, b.EqualTo(&a));
    ASSERT_ARE_EQUAL(int, false, a.EqualTo(&b));
}


TEST_FUNCTION(CMockValue_TN_setValue_from_NULL_to_NULL)
{
    CMockValue<arrayOf3Int> a(NULL);
    a.SetValue(NULL);

    ASSERT_ARE_EQUAL(char_ptr, "NULL", a.ToString().c_str());
}

TEST_FUNCTION(CMockValue_TN_setValue_from_NULL_to_array)
{
    arrayOf3Int source1 = { 1, 2, 3 };
    CMockValue<arrayOf3Int> a(NULL);
    a.SetValue(source1);

    ASSERT_ARE_EQUAL(char_ptr, "{1,2,3}", a.ToString().c_str());
}

TEST_FUNCTION(CMockValue_TN_setValue_from_array_to_NULL)
{
    arrayOf3Int source1 = { 1, 2, 3 };
    CMockValue<arrayOf3Int> a(source1);
    a.SetValue(NULL);

    ASSERT_ARE_EQUAL(char_ptr, "NULL", a.ToString().c_str());
}

TEST_FUNCTION(CMockValue_TN_setValue_from_array_to_array)
{
    arrayOf3Int source1 = { 1, 2, 3 };
    arrayOf3Int source2 = { 4, 5, 6 };
    CMockValue<arrayOf3Int> a(source1);
    a.SetValue(source2);

    ASSERT_ARE_EQUAL(char_ptr, "{4,5,6}", a.ToString().c_str());
}

TEST_FUNCTION(CMockValue_TN_getValue_from_NULL_returns_NULL)
{
    CMockValue<arrayOf3Int> a(NULL);

    ASSERT_ARE_EQUAL(int, 1, ((void*)NULL==(void*)a.GetValue())?1:0);
}

TEST_FUNCTION(CMockValue_TN_getValue_from_something_returns_equal_array)
{
    arrayOf3Int source1 = { 1, 2, 3 };
    CMockValue<arrayOf3Int> a(source1);

    const int* v = a.GetValue();

    for (size_t i = 0; i < 3; i++)
    {
        ASSERT_ARE_EQUAL(int, v[i], source1[i]);
    }
}

TEST_FUNCTION(CMockValue_TN_setValue_from_array_to_array_does_not_destroy_original_array)
{
    arrayOf3Int source1 = { 1, 2, 3 };
    arrayOf3Int source2 = { 4, 5, 6 };
    CMockValue<arrayOf3Int> a(source1);
    a.SetValue(source2);

    ASSERT_ARE_EQUAL(char_ptr, "{4,5,6}", a.ToString().c_str());
    
    ASSERT_ARE_EQUAL(int, 1, source1[0]);
    ASSERT_ARE_EQUAL(int, 2, source1[1]);
    ASSERT_ARE_EQUAL(int, 3, source1[2]);

    ASSERT_ARE_EQUAL(int, 4, source2[0]);
    ASSERT_ARE_EQUAL(int, 5, source2[1]);
    ASSERT_ARE_EQUAL(int, 6, source2[2]);

}



END_TEST_SUITE(CMockValue_tests)