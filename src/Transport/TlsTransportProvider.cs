// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;

    public class TlsTransportProvider : TransportProvider
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
            if (innerTransport.GetType() != typeof(TcpTransport))
            {
                throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpTransportUpgradeNotAllowed,
                    innerTransport.GetType().Name, typeof(TlsTransport).Name));
            }

            return new TlsTransport(innerTransport, this.tlsSettings);
        }
    }
}
