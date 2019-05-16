// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// Defines the AMQP protocol ID.
    /// </summary>
    public enum ProtocolId : byte
    {
        /// <summary>
        /// AMQP protocol.
        /// </summary>
        Amqp = 0,
        /// <summary>
        /// AMQP TLS upgrade protocol.
        /// </summary>
        AmqpTls = 2,
        /// <summary>
        /// AMQP SASL protocol.
        /// </summary>
        AmqpSasl = 3
    }
}
