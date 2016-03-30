// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;

    public sealed class TlsTransportProvider : TransportProvider
    {
        TlsTransportSettings tlsSettings;

        public TlsTransportProvider(TlsTransportSettings tlsSettings)
        {
            this.tlsSettings = tlsSettings;
            this.ProtocolId = ProtocolId.AmqpTls;
        }

        public TlsTransportSettings Settings
        {
            get { return this.tlsSettings; }
        }

        public override string ToString()
        {
            return "tls-provider";
        }

        protected override TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            return new TlsTransport(innerTransport, this.tlsSettings);
        }
    }
}
