// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    /// <summary>
    /// Event args used for the <see cref="AmqpLinkTerminus.Expired"/> event.
    /// </summary>
    public class ExpireLinkTerminusEventArgs : EventArgs
    {
        /// <summary>
        /// The <see cref="AmqpLinkTerminus"/> that is expiring.
        /// </summary>
        public AmqpLinkTerminus LinkTerminus { get; internal set; }

        /// <summary>
        /// The link that was associated with the link terminus when the terminus was suspended and marked for expiration.
        /// </summary>
        public AmqpLink SuspendedLink { get; internal set; }
    }
}
