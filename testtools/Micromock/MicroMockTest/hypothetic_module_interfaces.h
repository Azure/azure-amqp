/*
Copyright (c) Microsoft Corporation.  All rights reserved.

Use of this source code is subject to the terms of the Microsoft end-user
license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
If you did not accept the terms of the EULA, you are not authorized to use
this source code. For a copy of the EULA, please see the LICENSE.RTF on your
install media.
*/

/*defines*/
#ifndef HYPOTHETIC_MODULE_INTERFACE_H
#define HYPOTHETIC_MODULE_INTERFACE_H

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/*types*/
typedef void(*pVoidFunction)(void);
typedef char* pChar;

/*variable exports*/
/*function exports*/
extern void whenzero(void);
extern int whenizero(void);
extern int whenone  (_In_ int i);
extern int whentwo  (_In_z_ pChar s, _In_ int i);
extern int whenthree(_In_ char c, _In_z_ pChar s, _In_ int i);
extern int whenfour (_In_ unsigned short int si, _In_ char c, _In_z_ pChar s, _In_ int i);
extern int whenfive (_In_opt_ pVoidFunction pVoid, _In_ unsigned short int si, _In_ char c, _In_z_ pChar s, _In_ int i);
extern int whensix  (_In_ char c1, _In_ char c2, _In_ char c3, _In_ char c4, _In_ char c5, _In_ char c6);

#ifdef __cplusplus
}
#endif

#endif
