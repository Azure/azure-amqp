// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the durability policy for a terminus.
    /// </summary>
    public enum TerminusDurability : uint
    {
        /// <summary>
        /// No terminus state is retained durably
        /// </summary>
        None = 0,
        /// <summary>
        /// Only the existence and configuration of the terminus is retained durably.
        /// </summary>
        Configuration = 1,
        /// <summary>
        /// In addition to the existence and configuration of the terminus,
        /// the unsettled state for durable messages is retained durably.
        /// </summary>
        UnsettledState = 2
    }
}
