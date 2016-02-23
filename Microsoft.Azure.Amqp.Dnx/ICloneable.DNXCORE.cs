// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if DNXCORE

// This interface doesn't exist in DNXCORE50, define it manually
namespace System
{
    public interface ICloneable
    {
        object Clone();
    }
}

#endif // DNXCORE
