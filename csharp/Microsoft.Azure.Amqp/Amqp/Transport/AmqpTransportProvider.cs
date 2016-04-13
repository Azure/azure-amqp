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

        protected override TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            return innerTransport;
        }
    }
}
