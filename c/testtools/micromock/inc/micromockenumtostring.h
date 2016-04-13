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

#ifndef MICROMOCKENUMTOSTRING_H
#define MICROMOCKENUMTOSTRING_H

#ifdef USE_TDD4CPP

#define MICROMOCK_ENUM_TO_STRING(EnumName, ...) \
namespace TDD \
{ \
    const wchar_t* EnumName##_Strings[] = { __VA_ARGS__ }; \
    template <> std::string ToString<std::string, EnumName>(const EnumName& q)  \
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
            template <> static std::wstring ToString < EnumName > (const EnumName & q)  \
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
