// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;

    /// <summary>
    /// Provides TLS transport upgrade.
    /// </summary>
    public class TlsTransportProvider : TransportProvider
    {
        TlsTransportSettings tlsSettings;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="tlsSettings">The TLS transport settings.</param>
        public TlsTransportProvider(TlsTransportSettings tlsSettings)
        {
            this.tlsSettings = tlsSettings;
            this.ProtocolId = ProtocolId.AmqpTls;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="tlsSettings">The TLS transport settings.</param>
        /// <param name="version">The supported version.</param>
        public TlsTransportProvider(TlsTransportSettings tlsSettings, AmqpVersion version)
            : this(tlsSettings)
        {
            this.Versions.Add(version);
        }

        /// <summary>
        /// Gets the TLS transport settings.
        /// </summary>
        public TlsTransportSettings Settings
        {
            get { return this.tlsSettings; }
        }

        /// <summary>
        /// Gets a string representation of the object.
        /// </summary>
        /// <returns>A string representation of the object.</returns>
        public override string ToString()
        {
            return "tls-provider";
        }

        /// <summary>
        /// Creates a TLS transport from the inner transport.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="isInitiator">true if it is the initiator, false otherwise.</param>
        /// <returns>A <see cref="TlsTransport"/>.</returns>
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
