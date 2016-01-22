// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public enum TerminusDurability : uint
    {
        None = 0,
        Configuration = 1,
        UnsettledState = 2
    }
}
