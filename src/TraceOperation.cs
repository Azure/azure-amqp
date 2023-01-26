// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// Defines the operations used in <see cref="AmqpTrace"/> log methods.
    /// </summary>
    public enum TraceOperation
    {
        /// <summary>
        /// Object initialization.
        /// </summary>
        Initialize = 0,
        /// <summary>
        /// Create operation.
        /// </summary>
        Create = 1,
        /// <summary>
        /// Delete operation.
        /// </summary>
        Delete = 2,
        /// <summary>
        /// Add operation.
        /// </summary>
        Add = 3,
        /// <summary>
        /// Remove operation.
        /// </summary>
        Remove = 4,
        /// <summary>
        /// Open operation.
        /// </summary>
        Open = 5,
        /// <summary>
        /// Close operation.
        /// </summary>
        Close = 6,
        /// <summary>
        /// Send operation.
        /// </summary>
        Send = 7,
        /// <summary>
        /// Receive operation.
        /// </summary>
        Receive = 8,
        /// <summary>
        /// Connect operation.
        /// </summary>
        Connect = 9,
        /// <summary>
        /// Accept operation.
        /// </summary>
        Accept = 10,
        /// <summary>
        /// Execute operation.
        /// </summary>
        Execute = 11,
        /// <summary>
        /// Bind operation.
        /// </summary>
        Bind = 12,
        /// <summary>
        /// Attach operation.
        /// </summary>
        Attach = 13,
        /// <summary>
        /// Abort operation.
        /// </summary>
        Abort = 14,
        /// <summary>
        /// Flow operation.
        /// </summary>
        Flow = 15,
        /// <summary>
        /// Register active link operation.
        /// </summary>
        ActiveLinkRegistered = 16,
        /// <summary>
        /// Update active link operation.
        /// </summary>
        ActiveLinkUpdated = 17,
        /// <summary>
        /// Expire link operation.
        /// </summary>
        ActiveLinkExpired = 18,
        /// <summary>
        /// Refresh active link operation.
        /// </summary>
        ActiveLinkRefreshed = 19,
        /// <summary>
        /// Link Recovery Negotiate operation.
        /// </summary>
        LinkRecoveryNegotiate = 20,
        /// <summary>
        /// Link Recovery Negotiate Result.
        /// </summary>
        LinkRecoveryNegotiateResult = 21,
        /// <summary>
        /// Terminus state on close
        /// </summary>
        TerminusStateOnClose = 22,
    }
}
