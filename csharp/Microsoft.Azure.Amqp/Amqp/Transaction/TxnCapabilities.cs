// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using Microsoft.Azure.Amqp.Encoding;

    static class TxnCapabilities
    {
        public static readonly AmqpSymbol LocalTransactions = "amqp:local-transactions";
        public static readonly AmqpSymbol DistributedTxn = "amqp:distributed-transactions";
        public static readonly AmqpSymbol PrototableTransactions = "amqp:prototable-transactions";
        public static readonly AmqpSymbol MultiTxnsPerSsn = "amqp:multi-txns-per-ssn";
        public static readonly AmqpSymbol MultiSsnsPerTxn = "amqp:multi-ssns-per-txn";
    }
}
