// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the expiry policy for a terminus.
    /// </summary>
    public class TerminusExpiryPolicy
    {
        /// <summary>
        /// The expiry timer starts when terminus is detached.
        /// </summary>
        public static readonly AmqpSymbol LinkDetach = "link-detach";
        /// <summary>
        /// The expiry timer starts when the most recently associated session is ended.
        /// </summary>
        public static readonly AmqpSymbol SessionEnd = "session-end";
        /// <summary>
        /// The expiry timer starts when most recently associated connection is closed.
        /// </summary>
        public static readonly AmqpSymbol ConnectionClose = "connection-close";
        /// <summary>
        /// The terminus never expires.
        /// </summary>
        public static readonly AmqpSymbol Never = "never";
    }
}
