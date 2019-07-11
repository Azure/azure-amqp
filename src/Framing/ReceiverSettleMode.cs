// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the settlement policy of a receiver.
    /// </summary>
    public enum ReceiverSettleMode : byte
    {
        /// <summary>
        /// The receiver will spontaneously settle all incoming transfers.
        /// </summary>
        First = 0,
        /// <summary>
        /// The receiver will only settle after sending the disposition to the sender and
        /// receiving a disposition indicating settlement of the delivery from the sender.
        /// </summary>
        Second = 1
    }
}
