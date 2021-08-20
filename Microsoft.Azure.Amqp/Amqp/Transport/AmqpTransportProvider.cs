// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    public sealed class AmqpTransportProvider : TransportProvider
    {
        public AmqpTransportProvider()
        {
            this.ProtocolId = ProtocolId.Amqp;
        }

        internal AmqpTransportProvider(AmqpVersion version)
            : this()
        {
            this.Versions.Add(version);
        }

        protected override TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            return innerTransport;
        }
    }
}
