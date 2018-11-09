// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    public enum SettleMode : byte
    {
        SettleOnSend,
        SettleOnReceive,
        SettleOnDispose
    }
}
