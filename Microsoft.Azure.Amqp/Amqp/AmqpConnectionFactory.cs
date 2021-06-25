// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp.Transport;

    public class AmqpConnectionFactory
    {
        TlsTransportSettings tlsSettings;
        public TlsTransportSettings TlsSettings
        {
            get
            {
                if (this.tlsSettings == null)
                {
                    this.tlsSettings = new TlsTransportSettings();
                }

                return this.tlsSettings;
            }
        }

        public Task<AmqpConnection> OpenConnectionAsync(string address)
        {
            return this.OpenConnectionAsync(address, AmqpConstants.DefaultTimeout);
        }

        public Task<AmqpConnection> OpenConnectionAsync(string address, TimeSpan timeout)
        {
            return this.OpenConnectionAsync(new Uri(address), timeout);
        }

        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, TimeSpan timeout)
        {
            return this.OpenConnectionAsync(addressUri, addressUri.UserInfo, null, timeout, CancellationToken.None);
        }

        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, TimeSpan timeout)
        {
            return this.OpenConnectionAsync(addressUri, null, saslHandler, timeout, CancellationToken.None);
        }

        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, CancellationToken cancellationToken)
        {
            return this.OpenConnectionAsync(addressUri, addressUri.UserInfo, null, TimeSpan.MaxValue, cancellationToken);
        }

        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, CancellationToken cancellationToken)
        {
            return this.OpenConnectionAsync(addressUri, null, saslHandler, TimeSpan.MaxValue, cancellationToken);
        }

        async Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, string userInfo, SaslHandler saslHandler, TimeSpan timeout, CancellationToken cancellationToken)
        {
#if !PCL
            if (saslHandler == null && !string.IsNullOrEmpty(userInfo))
            {
                string[] parts = userInfo.Split(':');
                if (parts.Length > 2)
                {
                    throw new ArgumentException("addressUri.UserInfo " + addressUri.UserInfo);
                }

                string userName = Uri.UnescapeDataString(parts[0]);
                string password = parts.Length > 1 ? Uri.UnescapeDataString(parts[1]) : string.Empty;

                saslHandler = new SaslPlainHandler() { AuthenticationIdentity = userName, Password = password };
            }
#endif

            TransportSettings transportSettings;
            if (addressUri.Scheme.Equals(AmqpConstants.SchemeAmqp, StringComparison.OrdinalIgnoreCase))
            {
                transportSettings = new TcpTransportSettings()
                {
                    Host = addressUri.Host,
                    Port = addressUri.Port > -1 ? addressUri.Port : AmqpConstants.DefaultPort
                };
            }
            else if (addressUri.Scheme.Equals(AmqpConstants.SchemeAmqps, StringComparison.OrdinalIgnoreCase))
            {
                TcpTransportSettings tcpSettings = new TcpTransportSettings()
                {
                    Host = addressUri.Host,
                    Port = addressUri.Port > -1 ? addressUri.Port : AmqpConstants.DefaultSecurePort
                };

                var tls = new TlsTransportSettings(tcpSettings) { TargetHost = addressUri.Host };
                if (this.tlsSettings != null)
                {
#if !PCL
                    tls.CertificateValidationCallback = this.tlsSettings.CertificateValidationCallback;
                    tls.CheckCertificateRevocation = this.tlsSettings.CheckCertificateRevocation;
                    tls.Certificate = this.tlsSettings.Certificate;
                    tls.Protocols = this.tlsSettings.Protocols;
#endif
                }

                transportSettings = tls;
            }
#if !PCL
            else if (addressUri.Scheme.Equals(WebSocketTransportSettings.WebSockets, StringComparison.OrdinalIgnoreCase) ||
                addressUri.Scheme.Equals(WebSocketTransportSettings.SecureWebSockets, StringComparison.OrdinalIgnoreCase))
            {
                transportSettings = new WebSocketTransportSettings() { Uri = addressUri };
            }
#endif
            else
            {
                throw new NotSupportedException(addressUri.Scheme);
            }

            AmqpSettings settings = new AmqpSettings();

            if (saslHandler != null)
            {
                // Provider for "AMQP3100"
                SaslTransportProvider saslProvider = new SaslTransportProvider();
                saslProvider.Versions.Add(new AmqpVersion(1, 0, 0));
                saslProvider.AddHandler(saslHandler);
                settings.TransportProviders.Add(saslProvider);
            }

            // Provider for "AMQP0100"
            AmqpTransportProvider amqpProvider = new AmqpTransportProvider();
            amqpProvider.Versions.Add(new AmqpVersion(new Version(1, 0, 0, 0)));
            settings.TransportProviders.Add(amqpProvider);

            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
            TransportBase transport = await initiator.ConnectAsync(timeoutHelper.RemainingTime(), cancellationToken).ConfigureAwait(false);

            try
            {
                AmqpConnectionSettings connectionSettings = new AmqpConnectionSettings()
                {
                    ContainerId = Guid.NewGuid().ToString(),
                    HostName = addressUri.Host
                };

                AmqpConnection connection = new AmqpConnection(transport, settings, connectionSettings);
                await Task.Factory.FromAsync(
                    (t, k, c, s) => ((AmqpConnection)s).BeginOpen(t, k, c, s),
                    r => ((AmqpConnection)r.AsyncState).EndOpen(r),
                    timeoutHelper.RemainingTime(),
                    cancellationToken,
                    connection)
                    .ConfigureAwait(false);

                return connection;
            }
            catch
            {
                transport.Abort();
                throw;
            }
        }
    }
}
