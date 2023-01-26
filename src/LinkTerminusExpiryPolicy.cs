// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// Determines when the link terminus object should be deleted and forgotten.
    /// </summary>
    public enum LinkTerminusExpiryPolicy
    {
        /// <summary>
        /// This would indicate that link recovery should not be applied.
        /// </summary>
        None,

        /// <summary>
        /// Delete and forget the link terminus when the <see cref="AmqpLink"/> is detached.
        /// </summary>
        LinkDetach,

        /// <summary>
        /// Delete and forget the link terminus when the <see cref="AmqpSession"/> is ended.
        /// </summary>
        SessionEnd,

        /// <summary>
        /// Delete and forget the link terminus when the <see cref="AmqpConnection"/> is closed.
        /// </summary>
        ConnectionClose,

        /// <summary>
        /// Keep the link terminus object for as long as possible.
        /// </summary>
        Never
    }
}
