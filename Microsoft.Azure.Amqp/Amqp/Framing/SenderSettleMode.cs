// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public enum SenderSettleMode : byte
    {
        Unsettled = 0,
        Settled = 1,
        Mixed = 2
    }
}
