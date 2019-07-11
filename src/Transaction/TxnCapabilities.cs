// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the AMQP transaction capabilities.
    /// </summary>
    public static class TxnCapabilities
    {
        /// <summary>
        /// Support local transactions.
        /// </summary>
        public static readonly AmqpSymbol LocalTransactions = "amqp:local-transactions";
        /// <summary>
        /// Support AMQP Distributed Transactions.
        /// </summary>
        public static readonly AmqpSymbol DistributedTxn = "amqp:distributed-transactions";
        /// <summary>
        /// Support AMQP Promotable Transactions.
        /// </summary>
        public static readonly AmqpSymbol PromotableTransactions = "amqp:promotable-transactions";
        /// <summary>
        /// Support multiple active transactions on a single session.
        /// </summary>
        public static readonly AmqpSymbol MultiTxnsPerSsn = "amqp:multi-txns-per-ssn";
        /// <summary>
        /// Support transactions whose txn-id is used across sessions on one connection.
        /// </summary>
        public static readonly AmqpSymbol MultiSsnsPerTxn = "amqp:multi-ssns-per-txn";
    }
}
