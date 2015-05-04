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

#ifndef MICROMOCKCOMMON_H
#define MICROMOCKCOMMON_H

#pragma once

#include "tchar.h"
#include "string"
#include "sstream"

#define COUNT_OF(a)     (sizeof(a) / sizeof((a)[0]))

namespace std
{
    typedef std::basic_string<TCHAR> tstring;
    typedef std::basic_ostringstream<TCHAR> tostringstream;
}

#endif // MICROMOCKCOMMON_H
