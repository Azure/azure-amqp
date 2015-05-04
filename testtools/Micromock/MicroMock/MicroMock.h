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

#ifndef MICROMOCK_H
#define MICROMOCK_H

#pragma once

#include "Mock.h"
#include "RuntimeMock.h"
#include "GlobalMock.h"
#include "ThreadSafeGlobalMock.h"
#include "MockMethodCallBase.h"
#include "MockMethodCall.h"
#include "MockCallRecorder.h"
#include "MockCallArgument.h"
#include "MockValue.h"
#include "MockValueBase.h"
#include "MockResultValue.h"
#include "DetoursHookClass.h"
#include "StrictUnorderedCallComparer.h"
#include "StrictOrderedCallComparer.h"
#include "NiceCallComparer.h"
#include "MicroMockException.h"
#include "MicroMockTestMutex.h"
#include "MicroMockEnumToString.h"

// Defines for ignored arguments
#define IGNORED_PTR_ARG (NULL)
#ifndef _MSC_VER
#define IGNORE (0)
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

#define INSTALL_HOOK_CLASS_NAME_CONCAT(id)      Install_Hook_##id
#define INSTALL_HOOK_CLASS_NAME(id)             INSTALL_HOOK_CLASS_NAME_CONCAT(id)

#define INSTALL_HOOK(functionToHook, functionToCall) \
class INSTALL_HOOK_CLASS_NAME(__LINE__) \
{ \
public: \
    INSTALL_HOOK_CLASS_NAME(__LINE__)() \
    { \
        dynamic_cast<CDetoursHookClass*>(GetSingleton())->InstallHook(functionToHook, functionToCall);\
    } \
    virtual ~INSTALL_HOOK_CLASS_NAME(__LINE__)() \
    { \
        dynamic_cast<CDetoursHookClass*>(GetSingleton())->UninstallHook(functionToHook, functionToCall);\
    } \
}; \
INSTALL_HOOK_CLASS_NAME(__LINE__) INSTALL_HOOK_CLASS_NAME(__LINE__); \

#define MOCK_METHOD_END(resultType, value)                                                  \
    resultType resultValue = value;                                                         \
    if (NULL != result) resultValue = dynamic_cast<CMockValue<resultType>*>(result)->GetValue(); \
    return resultValue;                                                                     \
}

#define MOCK_VOID_METHOD_END()                                                              \
    result = result;                                                                        \
}

#include "MicroMockCallMacros.h"

#endif // MICROMOCK_H
