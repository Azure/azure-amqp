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

#ifndef MOCKVALUEBASE_H
#define MOCKVALUEBASE_H

#pragma once

#include "stdafx.h"

class CMockValueBase
{
public:
    virtual ~CMockValueBase();

public:
    virtual _Check_return_ std::tstring ToString() const = 0;
    virtual _Must_inspect_result_ bool EqualTo(_In_ const CMockValueBase* right) = 0;
};

#endif // MOCKVALUEBASE_H
