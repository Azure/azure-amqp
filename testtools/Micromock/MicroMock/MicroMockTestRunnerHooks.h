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

#ifndef MICROMOCCKTESTRUNNERHOOKS_H
#define MICROMOCCKTESTRUNNERHOOKS_H

#pragma once

#ifndef MOCK_ASSERT

#include "stdafx.h"
#include "TestRunnerSwitcher.h"

#define MOCK_ASSERT(expected, actual, assertString)     ASSERT_ARE_EQUAL((expected), (actual), (assertString))
#define MOCK_FAIL(expression)                           ASSERT_FAIL(expression)
#define MOCK_THROW(mockException)                       throw(mockException)

#else // MOCK_ASSERT
#define MOCK_THROW(...)
#endif // MOCK_ASSERT

#endif // MICROMOCCKTESTRUNNERHOOKS_H
