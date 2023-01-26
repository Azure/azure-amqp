// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// Settings for the AMQP protocol.
    /// </summary>
    public sealed class AmqpSettings
    {
        List<TransportProvider> transportProviders;

        /// <summary>
        /// Initializes the settings.
        /// </summary>
        public AmqpSettings()
        {
            this.MaxConcurrentConnections = int.MaxValue;
            this.MaxLinksPerSession = int.MaxValue;
            this.DefaultLinkCredit = AmqpConstants.DefaultLinkCredit;
            this.AllowAnonymousConnection = true;
        }

        /// <summary>
        /// The maximum number of connections allowed.
        /// </summary>
        public int MaxConcurrentConnections
        {
            get;
            set;
        }

        /// <summary>
        /// The maximum number of sessions allowed in a connection.
        /// </summary>
        public int MaxLinksPerSession
        {
            get;
            set;
        }

        /// <summary>
        /// The default credit of receiving links.
        /// </summary>
        public uint DefaultLinkCredit
        {
            get;
            set;
        }

        /// <summary>
        /// Requires a secure transport (e.g. TLS) for the connection.
        /// </summary>
        public bool RequireSecureTransport
        {
            get;
            set;
        }

        /// <summary>
        /// Allows a connection that is not authenticated through an
        /// SASL mechanism.
        /// </summary>
        public bool AllowAnonymousConnection
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the list of transport providers that are used
        /// during connection creation.
        /// </summary>
        /// <remarks>
        /// After the initial transport is created, the connection
        /// initiator walks through the list and upgrades the transport.
        /// </remarks>
        public IList<TransportProvider> TransportProviders
        {
            get
            {
                if (this.transportProviders == null)
                {
                    this.transportProviders = new List<TransportProvider>();
                }

                return this.transportProviders;
            }
        }

        /// <summary>
        /// Gets or sets the runtime provider that handles the protocol requests.
        /// </summary>
        public IRuntimeProvider RuntimeProvider
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the terminus store implementation that is responsible for
        /// storing and retrieving Terminus details as well as the deliveries associated
        /// with the terminus.
        /// </summary>
        public IAmqpTerminusStore TerminusStore
        {
            get;
            set;
        }

        internal T GetTransportProvider<T>() where T : TransportProvider
        {
            if (this.transportProviders != null)
            {
                foreach (TransportProvider provider in this.transportProviders)
                {
                    if (provider is T)
                    {
                        return (T)provider;
                    }
                }
            }

            return null;
        }

        internal bool TryGetTransportProvider(ProtocolHeader header, out TransportProvider provider)
        {
            if (this.TransportProviders.Count == 0)
            {
                throw new ArgumentException(nameof(TransportProviders));
            }

            provider = null;
            foreach (TransportProvider transportProvider in this.TransportProviders)
            {
                if (transportProvider.ProtocolId == header.ProtocolId)
                {
                    provider = transportProvider;
                    return true;
                }
            }

            // Not found. Return the preferred one based on settings
            provider = this.GetDefaultProvider();
            return false;
        }

        internal ProtocolHeader GetDefaultHeader()
        {
            TransportProvider provider = this.GetDefaultProvider();
            return new ProtocolHeader(provider.ProtocolId, provider.DefaultVersion);
        }

        internal ProtocolHeader GetSupportedHeader(ProtocolHeader requestedHeader)
        {
            // Protocol id negotiation
            TransportProvider provider = null;
            if (!this.TryGetTransportProvider(requestedHeader, out provider))
            {
                return this.GetDefaultHeader();
            }

            // Protocol version negotiation
            AmqpVersion version;
            if (!provider.TryGetVersion(requestedHeader.Version, out version))
            {
                return new ProtocolHeader(provider.ProtocolId, provider.DefaultVersion);
            }

            return requestedHeader;
        }

        /// <summary>
        /// Clones the settings object.
        /// </summary>
        /// <returns>A new settings object.</returns>
        public AmqpSettings Clone()
        {
            AmqpSettings settings = new AmqpSettings();
            settings.DefaultLinkCredit = this.DefaultLinkCredit;
            settings.transportProviders = new List<TransportProvider>(this.TransportProviders);
            settings.RuntimeProvider = this.RuntimeProvider;
            settings.RequireSecureTransport = this.RequireSecureTransport;
            settings.AllowAnonymousConnection = this.AllowAnonymousConnection;
            settings.TerminusStore = this.TerminusStore;
            return settings;
        }

        internal void ValidateInitiatorSettings()
        {
            if (this.TransportProviders.Count == 0)
            {
                throw new ArgumentException(nameof(TransportProviders));
            }
        }

        internal void ValidateListenerSettings()
        {
            if (this.TransportProviders.Count == 0)
            {
                throw new ArgumentException(nameof(TransportProviders));
            }
        }

        TransportProvider GetDefaultProvider()
        {
            TransportProvider provider = null;
            if (this.RequireSecureTransport)
            {
                provider = this.GetTransportProvider<TlsTransportProvider>();
            }
            else if (!this.AllowAnonymousConnection)
            {
                provider = this.GetTransportProvider<SaslTransportProvider>();
            }
            else
            {
                provider = this.GetTransportProvider<AmqpTransportProvider>();
            }

            return provider;
        }
    }
}
