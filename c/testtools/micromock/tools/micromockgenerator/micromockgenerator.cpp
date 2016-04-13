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
#include "string"
#include "sstream"
#include "fstream"

namespace std
{
    typedef std::basic_string<TCHAR> tstring;
    typedef std::basic_ostringstream<TCHAR> tostringstream;
}

using namespace std;

const TCHAR* fileHeader = _T("/*\n")
_T("Microsoft Azure IoT Device Libraries\n")
_T("Copyright (c) Microsoft Corporation\n")
_T("All rights reserved.\n")
_T("MIT License\n")
_T("Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated \n")
_T("documentation files (the Software), to deal in the Software without restriction, including without limitation \n")
_T("the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, \n")
_T("and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n")
_T("\n")
_T("The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n")
_T("\n")
_T("THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED \n")
_T("TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL \n")
_T("THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF \n")
_T("CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS \n")
_T("IN THE SOFTWARE.\n")
_T("*/\n")
_T("\n")
_T("// THIS FILE IS AUTOGENERATED!\n")
_T("// DO NOT EDIT!\n")
_T("//\n")
_T("\n")
_T("#ifndef MICROMOCKCALLMACROS_H\n")
_T("#define MICROMOCKCALLMACROS_H\n")
_T("\n")
_T("#pragma once\n")
_T("\n");


void GenerateMockCallMacros(_In_ size_t supportedArgCount)
{
    FILE* outFile = NULL;

    _tfopen_s(&outFile, _T("..\\..\\inc\\MicroMockCallMacros.h"), _T("wt"));
    if (NULL != outFile)
    {
        _ftprintf(outFile, fileHeader);

        for (UINT currentArgCount = 0; currentArgCount <= supportedArgCount; currentArgCount++)
        {
            tostringstream argsWithValuesSignatureStream;
            tostringstream argAndValuesMacroStream;
            tstring argsWithValuesSignature;
            tstring argAndValuesMacroString;

            for (UINT i = 1; i <= currentArgCount; i++)
            {
                if (i > 1)
                {
                    argsWithValuesSignatureStream << _T(", ");
                }
                argsWithValuesSignatureStream << _T("arg") << i << _T("Type arg") << i << _T("Value");
                argAndValuesMacroStream << _T(", arg") << i << _T("Type, arg") << i << _T("Value");
            }

            argsWithValuesSignature = argsWithValuesSignatureStream.str();

            argAndValuesMacroString = argAndValuesMacroStream.str();

            _ftprintf(outFile, _T("#define MOCK_ANY_METHOD_%u(static_, STATIC_, prefix, resultType, name%s) \\\n"), currentArgCount, argAndValuesMacroString.c_str());
            _ftprintf(outFile, _T("static_ CMockMethodCall<resultType>& prefix Expected_##name(%s) \\\n"), argsWithValuesSignature.c_str());
            _ftprintf(outFile, _T("{ \\\n"));
            if (currentArgCount > 0)
            {
                _ftprintf(outFile, _T("    CMockCallArgumentBase* args[%u]; \\\n"), currentArgCount);
            }
            for (UINT i = 1; i <= currentArgCount; i++)
            {
                _ftprintf(outFile, _T("    args[%u] = new CMockCallArgument<arg%uType>(arg%uValue); \\\n"), i - 1, i, i);
            }
            _ftprintf(outFile, _T("    CMockMethodCall<resultType>* mockMethodCall = \\\n"));
            _ftprintf(outFile, _T("        new CMockMethodCall<resultType>(_T(#name), %u, %s); \\\n"), currentArgCount, (currentArgCount > 0) ? _T("args") : _T("NULL"));
            _ftprintf(outFile, _T("        RECORD_EXPECTED_##STATIC_##MOCK_CALL(mockMethodCall); \\\n"));
            _ftprintf(outFile, _T("    return *mockMethodCall; \\\n"));
            _ftprintf(outFile, _T("} \\\n"));
            _ftprintf(outFile, _T("static_ resultType prefix name(%s) \\\n"), argsWithValuesSignature.c_str());
            _ftprintf(outFile, _T("{ \\\n"));
            if (currentArgCount > 0)
            {
                _ftprintf(outFile, _T("    CMockCallArgumentBase* args[%u]; \\\n"), currentArgCount);
            }
            for (UINT i = 1; i <= currentArgCount; i++)
            {
                _ftprintf(outFile, _T("    args[%u] = new CMockCallArgument<arg%uType>(arg%uValue); \\\n"), i - 1, i, i);
            }
            _ftprintf(outFile, _T("    CMockMethodCallBase* mockMethodCall = \\\n"));
            _ftprintf(outFile, _T("        new CMockMethodCall<resultType>(_T(#name), %u, %s); \\\n"), currentArgCount, (currentArgCount > 0) ? _T("args") : _T("NULL"));
            _ftprintf(outFile, _T("        CMockValueBase* result = RECORD_ACTUAL_##STATIC_##MOCK_CALL(mockMethodCall); \n"));
            _ftprintf(outFile, _T("\n"));

            // regular member function mock
            _ftprintf(outFile, _T("#define MOCK_METHOD_%u(prefix, resultType, name%s) \\\n"), currentArgCount, argAndValuesMacroString.c_str());
            _ftprintf(outFile, _T("MOCK_ANY_METHOD_%u(,,prefix, resultType, name%s) \n"), currentArgCount, argAndValuesMacroString.c_str());
            _ftprintf(outFile, _T("\n"));

            // static function mock
            _ftprintf(outFile, _T("#define MOCK_STATIC_METHOD_%u(prefix, resultType, name%s) \\\n"), currentArgCount, argAndValuesMacroString.c_str());
            _ftprintf(outFile, _T("MOCK_ANY_METHOD_%u(static, STATIC_, prefix, resultType, name%s) \n"), currentArgCount, argAndValuesMacroString.c_str());
            _ftprintf(outFile, _T("\n"));

            // global mock declaration
            _ftprintf(outFile, _T("#define DECLARE_GLOBAL_MOCK_METHOD_%u(mockClass, prefix, resultType, name%s) \\\n"), currentArgCount, argAndValuesMacroString.c_str());
            _ftprintf(outFile, _T("prefix resultType name(%s) \\\n"), argsWithValuesSignature.c_str());
            _ftprintf(outFile, _T("{ \\\n"));
            _ftprintf(outFile, _T("    return mockClass::name("));
            for (UINT i = 1; i <= currentArgCount; i++)
            {
                if (i > 1)
                {
                    _ftprintf(outFile, _T(","));
                }
                _ftprintf(outFile, _T("arg%uValue"), i);
            }
            _ftprintf(outFile, _T(");\\\n"));
            
            _ftprintf(outFile, _T("} \\\n"));
            _ftprintf(outFile, _T("\n"));
        }

        _ftprintf(outFile, _T("#endif // MICROMOCKCALLMACROS_H\n"));
        _ftprintf(outFile, _T("\n"));

        fclose(outFile);
    }
}

const char* CfileHeader = "/*\n"
"\n"
"Microsoft Azure IoT Device Libraries\n"
"Copyright (c) Microsoft Corporation\n"
"All rights reserved.\n"
"MIT License\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated \n"
"documentation files (the Software), to deal in the Software without restriction, including without limitation \n"
"the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, \n"
"and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED \n"
"TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL \n"
"THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF \n"
"CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS \n"
"IN THE SOFTWARE.\n"
"*/\n"
"\n"
"// THIS FILE IS AUTOGENERATED!\n"
"// DO NOT EDIT!\n"
"\n"
"\n"
"#ifndef TIMEDISCRETEMICROMOCKCALLMACROS_H\n"
"#define TIMEDISCRETEMICROMOCKCALLMACROS_H\n"
"\n"
"#pragma once\n"
"\n";

#define tab (char)(9)
void GenerateTimeDiscreteMockCallMacros(_In_ size_t supportedArgCount)
{
    unsigned int i,j;
    /*intended as ANSI, non TCHAR, since it produces C++ source code*/
    ofstream fout("..\\..\\inc\\TimeDiscreteMicroMockCallMacros.h");
    fout<<CfileHeader<<endl;
    for(i=0;i<supportedArgCount;i++)
    {
        fout<<"#define MOCK_TD_METHOD_"<<i<<"(prefix, resultType, name";
        
        //write the argument list... 
        for(j=1;j<=i;j++)
        {
            fout<<", arg"<<j<<"Type, arg"<<j<<"Value";
        }
        //close the argument list
        fout<<") \\"<<endl;

        //MOCK_METHOD1... same thing
        fout<<tab<<"MOCK_METHOD_"<<i<<"(prefix, resultType, name";

        for(j=1;j<=i;j++)
        {
            fout<<", arg"<<j<<"Type, arg"<<j<<"Value";
        }
        fout<<")\\"<<endl;

        fout<<tab<<"mockMethodCall->AddExtraCallArgument(new CMockCallArgument<UINT32>(MOCK_TIMEPROVIDER(__FUNCTION__)));\\"<<endl;
        fout<<tab<<"result = REMATCH_ACTUAL_STATIC_MOCK_CALL(mockMethodCall);\\"<<endl;
        fout<<endl;


        fout << "#define MOCK_STATIC_TD_METHOD_" << i << "(prefix, resultType, name";

        //write the argument list... 
        for (j = 1; j <= i; j++)
        {
            fout << ", arg" << j << "Type, arg" << j << "Value";
        }
        //close the argument list
        fout << ") \\" << endl;

        //MOCK_METHOD_n ... same thing
        fout << tab << "MOCK_STATIC_METHOD_" << i << "(prefix, resultType, name";

        for (j = 1; j <= i; j++)
        {
            fout << ", arg" << j << "Type, arg" << j << "Value";
        }
        fout << ")\\" << endl;

        fout << tab << "mockMethodCall->AddExtraCallArgument(new CMockCallArgument<UINT32>(MOCK_TIMEPROVIDER(__FUNCTION__)));\\" << endl;
        fout << tab << "result = REMATCH_ACTUAL_STATIC_MOCK_CALL(mockMethodCall);\\" << endl;
        fout << endl;
    }

    fout<<"#define STIM_CALL_AT(var, time, ...)	var.__VA_ARGS__.SetTime(time, var.getAndIncOrder(time))"<<endl;
    fout<<endl;

    for(i=0;i<supportedArgCount;i++)
    {
        fout<<"#define DECLARE_STIM_STATIC_TD_METHOD_"<<i<<"(prefix, resultType, name";
        for(j=1;j<=i;j++)
        {
            fout<<", arg"<<j<<"Type, arg"<<j<<"Value";
        }
        fout<<")\\"<<endl;
        fout<<"class pFunctionCall_Wrapper_##name \\"<<endl;
        fout<<"{\\"<<endl;
        fout<<tab<<"public:\\"<<endl;
        fout<<tab<<"typedef resultType (*pRealFunctionType)(";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type arg"<<j<<"Value";
            if((i>0) &&(j<i))fout<<", ";
        }
        fout<<");\\"<<endl;
        fout<<tab<<"static pRealFunctionType realFunction;\\"<<endl;
        fout<<"};\\"<<endl;

        fout<<"pFunctionCall_Wrapper_##name::pRealFunctionType pFunctionCall_Wrapper_##name::realFunction = ::name; /*so this never can go to a header*/"<<endl;
        fout<<endl;

        fout<<"#define STIM_STATIC_TD_METHOD_"<<i<<"(prefix, resultType, name ";
        for(j=1;j<=i;j++)
        {
            fout<<", arg"<<j<<"Type, arg"<<j<<"Value";
        }
        fout<<") call"<<i<<"Arg<resultType, ";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type, ";
        }
        fout<<"pFunctionCall_Wrapper_##name > name;"<<endl;
        fout<<endl;
    }

    for(i=0;i<supportedArgCount;i++)
    {
        fout<<"template<typename resultType,";
        for(j=1;j<=i;j++)
        {
            fout<<"typename arg"<<j<<"Type, ";
        }
        fout<<"class C>";
        fout<<"class call"<<i<<"Arg : public canPlay"<<endl;
        fout<<"{"<<endl;
        fout<<"private:"<<endl;
        fout<<tab<<"class timeS"<<endl;
        fout<<tab<<"{"<<endl;
        fout<<tab<<"public:"<<endl;
        fout<<tab<<tab<<"timeS(";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type arg"<<j<<"Value";
            if((i>0) && (j<i)) fout<<", ";
        }
        fout<<"): "<<endl;
        fout<<tab<<tab<<"time(0), order(0)";
        for(j=1;j<=i;j++)
        {
            fout<<", arg"<<j<<"(arg"<<j<<"Value)";
        }
        fout<<endl;
        fout<<tab<<tab<<"{"<<endl;
        fout<<tab<<tab<<"}"<<endl;
        fout<<tab<<tab<<"UINT32 time;"<<endl;
        fout<<tab<<tab<<"UINT32 order;"<<endl;
        for(j=1;j<=i;j++)
        {
            fout<<tab<<tab<<"valueHolder<arg"<<j<<"Type> arg"<<j<<";"<<endl;
        }
        fout<<tab<<"};"<<endl;

        fout<<tab<<"std::vector<timeS> allCalls;"<<endl;

        fout<<"public:"<<endl;
        fout<<tab<<"call"<<i<<"Arg()"<<endl;
        fout<<tab<<"{"<<endl;
        fout<<tab<<tab<<"stims_base::registerCallXArg(this);"<<endl;
        fout<<tab<<"}"<<endl;
        fout<<endl;
    
        fout<<tab<<"virtual ~call"<<i<<"Arg<resultType, ";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type, ";
        }
        fout<<" C>()"<<endl;
        fout<<tab<<"{"<<endl;
        fout<<tab<<"}"<<endl;


        fout<<tab<<"virtual void PlayTick(_In_ UINT32 tick, _In_ UINT32 order)"<<endl;
        fout<<tab<<"{"<<endl;
        fout<<tab<<tab<<"for(UINT32 i=0; i<allCalls.size();i++)"<<endl;
        fout<<tab<<tab<<"{"<<endl;
        fout<<tab<<tab<<tab<<"if((allCalls[i].time==tick)&&(allCalls[i].order==order))"<<endl;
        fout<<tab<<tab<<tab<<"{"<<endl;
        fout<<tab<<tab<<tab<<tab<<"C::realFunction(";
        for(j=1;j<=i;j++)
        {
            fout<<"allCalls[i].arg"<<j;
            if((i>0)&&(j<i)) fout<<", ";
        }
        fout<<");"<<endl;
        fout<<tab<<tab<<tab<<"}"<<endl;
        fout<<tab<<tab<<"}"<<endl;
        fout<<tab<<"}"<<endl;

        fout<<endl;

        fout<<tab<<"call"<<i<<"Arg<resultType, ";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type, ";
        }
        fout<<"C>& SetTime(_In_ UINT32 time, _In_ UINT32 order)"<<endl;
        fout<<tab<<"{"<<endl;
        fout<<tab<<tab<<"if(allCalls.size()==0)"<<endl;
        fout<<tab<<tab<<"{"<<endl;
        fout<<tab<<tab<<tab<<"throw CMicroMockException(MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL, _T(\"using SetTime before the call has been defined usually indicates an error in test code\"));"<<endl;
        fout<<tab<<tab<<"}"<<endl;
        fout<<tab<<tab<<"else"<<endl;
        fout<<tab<<tab<<"{"<<endl;
        fout<<tab<<tab<<tab<<"allCalls[allCalls.size()-1].time=time;"<<endl;
        fout<<tab<<tab<<tab<<"allCalls[allCalls.size()-1].order=order;"<<endl;
        fout<<tab<<tab<<"}"<<endl;
        fout<<tab<<tab<<"return *this;"<<endl;
        fout<<tab<<"}"<<endl;
        fout<<endl;

        fout<<tab<<"call"<<i<<"Arg<resultType, ";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type, ";
        }
        fout<<" C>& operator()(";
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j<<"Type arg"<<j;
            if((i>0)&&(j<i)) fout<<",";
        }
        fout<<")"<<endl;
        fout<<tab<<"{"<<endl;
        fout<<tab<<tab<<"timeS s"<<((i==0)?"":"(");
        for(j=1;j<=i;j++)
        {
            fout<<"arg"<<j;
            if((i>0)&&(j<i)) fout<<", ";
        }
        fout<<((i==0)?"":")")<<";"<<endl;
        fout<<tab<<tab<<"allCalls.push_back(s); /*time is updated \"later\" by chaining*/"<<endl;
        fout<<tab<<tab<<"return *this;"<<endl;
        fout<<tab<<"}"<<endl;
        fout<<endl;

        if(i>0)
        {
            fout<<tab<<"call"<<i<<"Arg<resultType, ";
            for(j=1;j<=i;j++)
            {
                fout<<"arg"<<j<<"Type, ";
            }
            fout<<" C>& setArraySize(_In_ UINT32 parameter, _In_ size_t nElements)"<<endl;
            fout<<tab<<"{"<<endl;
            fout<<tab<<tab<<"if(allCalls.size()==0)"<<endl;
            fout<<tab<<tab<<"{"<<endl;	
            fout<<tab<<tab<<tab<<"throw CMicroMockException(MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL, _T(\"using setArraySize before the call has been defined usually indicates an error in test code\"));"<<endl;
            fout<<tab<<tab<<"}"<<endl;
            fout<<tab<<tab<<"else"<<endl;
            fout<<tab<<tab<<"{"<<endl;
            fout<<tab<<tab<<tab<<"timeS& s = allCalls[allCalls.size()-1]; /*get the last element*/"<<endl;
            fout<<tab<<tab<<tab<<"switch(parameter)"<<endl;
            fout<<tab<<tab<<tab<<"{"<<endl;
            for(j=1;j<=i;j++)
            {
                fout<<tab<<tab<<tab<<tab<<"case "<<j<<":"<<endl;
                fout<<tab<<tab<<tab<<tab<<"{"<<endl;
                fout<<tab<<tab<<tab<<tab<<tab<<"s.arg"<<j<<".setArraySize(nElements);"<<endl;
                fout<<tab<<tab<<tab<<tab<<tab<<"break;"<<endl;
                fout<<tab<<tab<<tab<<tab<<"}"<<endl;
            }
            fout<<tab<<tab<<tab<<tab<<"default:"<<endl;
            fout<<tab<<tab<<tab<<tab<<"{"<<endl;
            fout<<tab<<tab<<tab<<tab<<tab<<"ASSERT_FAIL(_T(\"there are no parameters so big\"));"<<endl;
            fout<<tab<<tab<<tab<<tab<<"}"<<endl;
            fout<<tab<<tab<<tab<<"}"<<endl;
            fout<<tab<<tab<<tab<<"return *this;"<<endl;
            fout<<tab<<tab<<"}"<<endl;
            fout<<tab<<"}"<<endl;
        }
        fout<<"};"<<endl;
        fout<<endl;
    }

    fout<<"#endif // TIMEDISCRETEMICROMOCKCALLMACROS_H"<<endl;
    fout<<endl;

    fout.close();
}

int __cdecl _tmain(_In_ const int argc, _In_reads_(argc) const _TCHAR* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    size_t supportedArgCount = 16;

    _tprintf(_T("MicroMock code generator.\n"));

    _tprintf(_T("Generating MicroMock call macros...\n"));
    GenerateMockCallMacros(supportedArgCount);

    _tprintf(_T("Generating TimeDiscreteMicroMock call macros...\n"));
    GenerateTimeDiscreteMockCallMacros(supportedArgCount);

    _tprintf(_T("Done.\n"));

    return 0;
}

