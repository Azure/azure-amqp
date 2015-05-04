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

#ifndef MICROMOCKENUMTOSTRING_H
#define MICROMOCKENUMTOSTRING_H

#ifdef USE_TDD4CPP

#define MICROMOCK_ENUM_TO_STRING(EnumName, ...) \
namespace TDD \
{ \
    const wchar_t* EnumName##_Strings[] = { __VA_ARGS__ }; \
    template <> std::string ToString<std::string, enum EnumName>(const enum EnumName& q)  \
    { \
        if((size_t)q >= sizeof(EnumName##_Strings)/sizeof(EnumName##_Strings[0])) \
        { \
            Assert::Fail(L"out of range value for " #EnumName); \
            return ""; \
        } \
        else \
        { \
            const wchar_t* w = EnumName##_Strings[q]; \
            std::setlocale(LC_ALL, ""); \
            std::wstring t(w); \
            std::string s(t.length()+1, 0); \
            std::locale loc(""); \
            std::use_facet < std::ctype < wchar_t > >(loc).narrow(w, &w[t.length()], '\0', &s[0]); \
            s.resize(t.length()); \
            return s; \
        } \
    } \
};

#elif defined USE_CTEST

#define MICROMOCK_ENUM_TO_STRING(EnumName, ...) \
const wchar_t *EnumName##_Strings[]= \
{ \
__VA_ARGS__ \
}; \
static void EnumName##_ToString(char* dest, size_t bufferSize, EnumName enumValue) \
{ \
    (void)snprintf(dest, bufferSize, "%S", EnumName##_Strings[enumValue]); \
} \
static bool EnumName##_Compare(EnumName left, EnumName right) \
{ \
    return left != right; \
}

#else

#define MICROMOCK_ENUM_TO_STRING(EnumName, ...) \
namespace Microsoft \
{ \
    namespace VisualStudio \
    { \
        namespace CppUnitTestFramework \
        { \
            static const wchar_t *EnumName##_Strings[]= \
            { \
                __VA_ARGS__ \
            }; \
            template <> static std::wstring ToString < enum EnumName > (const enum EnumName & q)  \
            {  \
                if((size_t)q>=sizeof(EnumName##_Strings)/sizeof(EnumName##_Strings[0])) \
                { \
                    Assert::Fail(L"out of range value for " L#EnumName); \
                    return L""; \
                } \
                else \
                { \
                    return EnumName##_Strings[q]; \
                } \
            } \
        } \
    } \
};

#endif

#endif
