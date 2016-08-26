// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp.Transport;

    public sealed class AmqpSettings
    {
        List<TransportProvider> transportProviders;

        public AmqpSettings()
        {
            this.MaxConcurrentConnections = int.MaxValue;
            this.MaxLinksPerSession = int.MaxValue;
            this.DefaultLinkCredit = AmqpConstants.DefaultLinkCredit;
            this.AllowAnonymousConnection = true;
        }

        public int MaxConcurrentConnections
        {
            get;
            set;
        }

        public int MaxLinksPerSession
        {
            get;
            set;
        }

        public uint DefaultLinkCredit
        {
            get;
            set;
        }

        public bool RequireSecureTransport
        {
            get;
            set;
        }

        public bool AllowAnonymousConnection
        {
            get;
            set;
        }

        /// <summary>
        /// Providers should be added in preferred order.
        /// </summary>
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

        public IRuntimeProvider RuntimeProvider
        {
            get;
            set;
        }

        public T GetTransportProvider<T>() where T : TransportProvider
        {
            foreach (TransportProvider provider in this.transportProviders)
            {
                if (provider is T)
                {
                    return (T)provider;
                }
            }

            return null;
        }

        public bool TryGetTransportProvider(ProtocolHeader header, out TransportProvider provider)
        {
            if (this.TransportProviders.Count == 0)
            {
                throw new ArgumentException("TransportProviders");
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

        public ProtocolHeader GetDefaultHeader()
        {
            TransportProvider provider = this.GetDefaultProvider();
            return new ProtocolHeader(provider.ProtocolId, provider.DefaultVersion);
        }

        public ProtocolHeader GetSupportedHeader(ProtocolHeader requestedHeader)
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

        public AmqpSettings Clone()
        {
            AmqpSettings settings = new AmqpSettings();
            settings.DefaultLinkCredit = this.DefaultLinkCredit;
            settings.transportProviders = new List<TransportProvider>(this.TransportProviders);
            settings.RuntimeProvider = this.RuntimeProvider;
            settings.RequireSecureTransport = this.RequireSecureTransport;
            settings.AllowAnonymousConnection = this.AllowAnonymousConnection;
            return settings;
        }

        public void ValidateInitiatorSettings()
        {
            if (this.TransportProviders.Count == 0)
            {
                throw new ArgumentException("TransportProviders");
            }
        }

        public void ValidateListenerSettings()
        {
            if (this.TransportProviders.Count == 0)
            {
                throw new ArgumentException("TransportProviders");
            }
        }

        TransportProvider GetDefaultProvider()
        {
#if !PCL
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
#else
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
#endif
        }
    }
}
