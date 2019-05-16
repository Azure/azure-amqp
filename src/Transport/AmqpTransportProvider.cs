// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    /// <summary>
    /// Provides AMQP transport upgrade.
    /// </summary>
    public sealed class AmqpTransportProvider : TransportProvider
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        public AmqpTransportProvider()
        {
            this.ProtocolId = ProtocolId.Amqp;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="version">The supported version.</param>
        public AmqpTransportProvider(AmqpVersion version)
            : this()
        {
            this.Versions.Add(version);
        }

        /// <summary>
        /// Creates a transport from the inner transport to transfer AMQP frames.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="isInitiator">true if it is the initiator, false otherwise.</param>
        /// <returns>The AMQP transport.</returns>
        protected override TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            return innerTransport;
        }
    }
}
