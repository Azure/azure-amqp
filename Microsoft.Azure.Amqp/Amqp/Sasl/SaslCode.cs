// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    public enum SaslCode : byte
    {
        Ok = 0,
        Auth = 1,
        Sys = 2,
        SysPerm = 3,
        SysTemp = 4
    }
}
