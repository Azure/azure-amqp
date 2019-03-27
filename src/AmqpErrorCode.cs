// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the standard AMQP error codes.
    /// </summary>
    public static class AmqpErrorCode
    {
#pragma warning disable 1591
        // amqp errors
        public static AmqpSymbol InternalError = "amqp:internal-error";
        public static AmqpSymbol NotFound = "amqp:not-found";
        public static AmqpSymbol UnauthorizedAccess = "amqp:unauthorized-access";
        public static AmqpSymbol DecodeError = "amqp:decode-error";
        public static AmqpSymbol ResourceLimitExceeded = "amqp:resource-limit-exceeded";
        public static AmqpSymbol NotAllowed = "amqp:not-allowed";
        public static AmqpSymbol InvalidField = "amqp:invalid-field";
        public static AmqpSymbol NotImplemented = "amqp:not-implemented";
        public static AmqpSymbol ResourceLocked = "amqp:resource-locked";
        public static AmqpSymbol PreconditionFailed = "amqp:precondition-failed";
        public static AmqpSymbol ResourceDeleted = "amqp:resource-deleted";
        public static AmqpSymbol IllegalState = "amqp:illegal-state";
        public static AmqpSymbol FrameSizeTooSmall = "amqp:frame-size-too-small";

        // connection errors
        public static AmqpSymbol ConnectionForced = "amqp:connection:forced";
        public static AmqpSymbol FramingError = "amqp:connection:framing-error";
        public static AmqpSymbol ConnectionRedirect = "amqp:connection:redirect";
        
        // session errors
        public static AmqpSymbol WindowViolation = "amqp:session:window-violation";
        public static AmqpSymbol ErrantLink = "amqp:session-errant-link";
        public static AmqpSymbol HandleInUse = "amqp:session:handle-in-use";
        public static AmqpSymbol UnattachedHandle = "amqp:session:unattached-handle";

        // link errors
        public static AmqpSymbol DetachForced = "amqp:link:detach-forced";
        public static AmqpSymbol TransferLimitExceeded = "amqp:link:transfer-limit-exceeded";
        public static AmqpSymbol MessageSizeExceeded = "amqp:link:message-size-exceeded";
        public static AmqpSymbol LinkRedirect = "amqp:link:redirect";
        public static AmqpSymbol Stolen = "amqp:link:stolen";

        // tx error conditions
        public static AmqpSymbol TransactionUnknownId = "amqp:transaction:unknown-id";
        public static AmqpSymbol TransactionRollback = "amqp:transaction:rollback";
        public static AmqpSymbol TransactionTimeout = "amqp:transaction:timeout";
#pragma warning restore 1591
    }
}
