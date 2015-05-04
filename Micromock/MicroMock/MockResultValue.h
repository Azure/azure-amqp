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

#ifndef MOCKRESULTVALUE_H
#define MOCKRESULTVALUE_H

#pragma once

#include "stdafx.h"
#include "MockValue.h"

template<typename T>
class CMockResultValue :
    public CMockValue<T>
{
public:
    CMockResultValue(T argValue) :
        CMockValue<T>(argValue)
    {
    }
};

#endif // MOCKRESULTVALUE_H
