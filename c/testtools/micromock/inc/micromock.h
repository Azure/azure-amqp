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

#ifndef MICROMOCK_H
#define MICROMOCK_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "mock.h"
#include "globalmock.h"
#include "threadsafeglobalmock.h"
#include "mockmethodcallbase.h"
#include "mockmethodcall.h"
#include "mockcallrecorder.h"
#include "mockcallargument.h"
#include "mockvalue.h"
#include "mockvaluebase.h"
#include "mockresultvalue.h"
#include "strictunorderedcallcomparer.h"
#include "strictorderedcallcomparer.h"
#include "nicecallcomparer.h"
#include "micromockexception.h"
#include "micromocktestmutex.h"
#include "micromockenumtostring.h"

// Defines for ignored arguments
#define IGNORED_PTR_ARG (NULL)
#define IGNORED_NUM_ARG (0)
#ifndef _MSC_VER
#define IGNORED_NUM_ARG (0)
#endif

#define TYPED_MOCK_CLASS(className, mockType)       \
class className : public CStrictUnorderedCallComparer< mockType<className> >

#define MICROMOCK_POINTER_TYPE_EQUAL(T)                             \
template<>                                                          \
static bool operator==<T*>(_In_ const CMockValue<T*>& lhs,          \
    _In_ const CMockValue<T*>& rhs)                                 \
{                                                                   \
    bool result = false;                                            \
    if (lhs.GetValue() == rhs.GetValue())                           \
    {                                                               \
        result = true;                                              \
    } else                                                          \
    if ((NULL != lhs.GetValue()) &&                                 \
        (NULL != rhs.GetValue()) &&                                 \
        (*lhs.GetValue() == *rhs.GetValue()))                       \
    {                                                               \
        result = true;                                              \
    }                                                               \
    return result;                                                  \
}

// A strict expected call implies that all arguments are checked
#define STRICT_EXPECTED_CALL(ClassName,  ...)        \
    ClassName.Expected_##__VA_ARGS__

// By using the below macro, none of the arguments are checked by default
// To specify checking the argument values, use the ValidateArgument 
// expected call modifier method
#define EXPECTED_CALL(ClassName, ...)               \
STRICT_EXPECTED_CALL(ClassName, __VA_ARGS__)        \
    .IgnoreAllArguments()

// When called only specifies actions for the specific call instance
// he actions specified this way are not included in the comparison
// of expected and actual calls
#define WHEN_CALLED(ClassName, ...)          \
EXPECTED_CALL(ClassName, __VA_ARGS__)        \
    .OnlySpecifiesActions()

// This is the strict version of WHEN_CALLED
#define WHEN_CALLED_STRICT(ClassName, ...)   \
STRICT_EXPECTED_CALL(ClassName, __VA_ARGS__) \
    .OnlySpecifiesActions()

#define RECORD_ACTUAL_MOCK_CALL(mockMethodCall)         \
    RecordActualCall(mockMethodCall)

#define RECORD_EXPECTED_MOCK_CALL(mockMethodCall)       \
    RecordExpectedCall(mockMethodCall)

#define REMATCH_ACTUAL_MOCK_CALL(mockMethodCall)        \
    MatchActualCall(mockMethodCall)

#define RECORD_ACTUAL_STATIC_MOCK_CALL(mockMethodCall)  \
    GetSingleton()->RecordActualCall(mockMethodCall)

#define RECORD_EXPECTED_STATIC_MOCK_CALL(mockMethodCall)\
    GetSingleton()->RecordExpectedCall(mockMethodCall)

#define REMATCH_ACTUAL_STATIC_MOCK_CALL(mockMethodCall) \
    GetSingleton()->MatchActualCall(mockMethodCall)

#define STATIC_MOCK_LOCK(name)                          \
void MockLock_##name()                                  \
{                                                       \
    GetSingleton()->Lock();                             \
}

#define STATIC_MOCK_UNLOCK()                            \
    GetSingleton()->Unlock()

#define MOCK_METHOD_END(resultType, value)                                                  \
    resultType resultValue = value;                                                         \
    if (NULL != result) resultValue = dynamic_cast<CMockValue<resultType>*>(result)->GetValue(); \
    return resultValue;                                                                     \
}

#define MOCK_VOID_METHOD_END()                                                              \
    result = result;                                                                        \
}

#include "micromockcallmacros.h"

#endif // MICROMOCK_H
