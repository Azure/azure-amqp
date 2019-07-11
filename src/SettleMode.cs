// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// Defines the settlement modes of a delivery.
    /// </summary>
    public enum SettleMode : byte
    {
        /// <summary>
        /// A delivery is settled when it is sent (at most once).
        /// </summary>
        SettleOnSend,
        /// <summary>
        /// A delivery is settled when it is received (at least once).
        /// </summary>
        SettleOnReceive,
        /// <summary>
        /// A delivery is settled when a disposition is received (exact once).
        /// </summary>
        SettleOnDispose
    }
}
