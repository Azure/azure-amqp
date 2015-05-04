/*
Copyright (c) Microsoft Corporation.  All rights reserved.

Use of this source code is subject to the terms of the Microsoft end-user
license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
If you did not accept the terms of the EULA, you are not authorized to use
this source code. For a copy of the EULA, please see the LICENSE.RTF on your
install media.
*/

#pragma once


// 4505 is disabled due to unused template specializations declared in CppUnitTest
#pragma warning(disable:4505)

// Headers for CppUnitTest
//#include "TestRunnerSwitcher.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include "MicroMock.h"
#include "TimeDiscreteMicroMock.h"
