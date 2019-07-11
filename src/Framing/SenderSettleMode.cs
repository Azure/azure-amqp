// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the settlement policy of a sender.
    /// </summary>
    public enum SenderSettleMode : byte
    {
        /// <summary>
        /// The sender will send all deliveries initially unsettled to the receiver.
        /// </summary>
        Unsettled = 0,
        /// <summary>
        /// The sender will send all deliveries settled to the receiver.
        /// </summary>
        Settled = 1,
        /// <summary>
        /// The sender MAY send a mixture of settled and unsettled deliveries to the receiver.
        /// </summary>
        Mixed = 2
    }
}
