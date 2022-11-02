// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// A factory to open AMQP connections.
    /// </summary>
    public class AmqpConnectionFactory
    {
        readonly AmqpSettings settings;
        TlsTransportSettings tlsSettings;

        /// <summary>
        /// Initializes a factory with default protocol settings.
        /// </summary>
        public AmqpConnectionFactory()
            : this(new AmqpSettings())
        {
        }

        /// <summary>
        /// Initializes a factory with protocol settings.
        /// </summary>
        /// <param name="settings"></param>
        public AmqpConnectionFactory(AmqpSettings settings)
        {
            this.settings = settings;
        }

        /// <summary>
        /// Gets the protocol settings of the factory.
        /// </summary>
        public AmqpSettings Settings => this.settings;

        /// <summary>
        /// Gets the TLS transport settings.
        /// </summary>
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

        /// <summary>
        /// Opens a connection to the specified address with a default operation timeout.
        /// </summary>
        /// <param name="address">The address.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(string address)
        {
            return this.OpenConnectionAsync(address, AmqpConstants.DefaultTimeout);
        }

        /// <summary>
        /// Opens a connection to the specified address.
        /// </summary>
        /// <param name="address">The Uri string of the address. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(string address, TimeSpan timeout)
        {
            return this.OpenConnectionAsync(new Uri(address), timeout);
        }

        /// <summary>
        /// Opens a connection to the specified address with a default operation timeout.
        /// </summary>
        /// <param name="addressUri">The address.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri)
        {
            return this.OpenConnectionAsync(addressUri, AmqpConstants.DefaultTimeout);
        }

        /// <summary>
        /// Opens a connection to the specified address.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, TimeSpan timeout)
        {
            SaslHandler saslHandler = CreateDefaultSaslHandlerIfPossible(addressUri);
            return this.OpenConnectionAsync(addressUri, saslHandler, null, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Opens a connection to the specified address using the provided <see cref="AmqpConnectionSettings"/>.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="connectionSettings">existing settings which may be taken from a previously existing connection.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, AmqpConnectionSettings connectionSettings, TimeSpan timeout)
        {
            SaslHandler saslHandler = CreateDefaultSaslHandlerIfPossible(addressUri);
            return this.OpenConnectionAsync(addressUri, saslHandler, connectionSettings, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Opens a connection to the specified address.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="saslHandler">The SASL handler to perform authentication.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, TimeSpan timeout)
        {
            return this.OpenConnectionAsync(addressUri, saslHandler, null, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Opens a connection to the specified address using the provided <see cref="AmqpConnectionSettings"/>.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="saslHandler">The SASL handler to perform authentication.</param>
        /// <param name="connectionSettings">existing settings which may be taken from a previously existing connection.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, AmqpConnectionSettings connectionSettings, TimeSpan timeout)
        {
            return this.OpenConnectionAsync(addressUri, saslHandler, connectionSettings, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Opens a connection to the specified address.
        /// </summary>
        /// <param name="address">The address of the remote peer.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(string address, CancellationToken cancellationToken)
        {
            return this.OpenConnectionAsync(new Uri(address), null, null, TimeSpan.MaxValue, cancellationToken);
        }

        /// <summary>
        /// Opens a connection to the specified address.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, CancellationToken cancellationToken)
        {
            SaslHandler saslHandler = CreateDefaultSaslHandlerIfPossible(addressUri);
            return this.OpenConnectionAsync(addressUri, saslHandler, null, TimeSpan.MaxValue, cancellationToken);
        }

        /// <summary>
        /// Opens a connection to the specified address using the provided <see cref="AmqpConnectionSettings"/>.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="connectionSettings">existing settings which may be taken from a previously existing connection.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, AmqpConnectionSettings connectionSettings, CancellationToken cancellationToken)
        {
            SaslHandler saslHandler = CreateDefaultSaslHandlerIfPossible(addressUri);
            return this.OpenConnectionAsync(addressUri, saslHandler, connectionSettings, TimeSpan.MaxValue, cancellationToken);
        }

        /// <summary>
        /// Opens a connection to the specified address.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="saslHandler">The SASL handler to perform authentication.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, CancellationToken cancellationToken)
        {
            return this.OpenConnectionAsync(addressUri, saslHandler, null, TimeSpan.MaxValue, cancellationToken);
        }

        /// <summary>
        /// Opens a connection to the specified address using the provided <see cref="AmqpConnectionSettings"/>.
        /// </summary>
        /// <param name="addressUri">The address Uri. If it contains user info, SASL PLAIN is enabled.</param>
        /// <param name="saslHandler">The SASL handler to perform authentication.</param>
        /// <param name="connectionSettings">existing settings which may be taken from a previously existing connection.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An AMQP connection.</returns>
        public Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, AmqpConnectionSettings connectionSettings, CancellationToken cancellationToken)
        {
            return this.OpenConnectionAsync(addressUri, saslHandler, connectionSettings, TimeSpan.MaxValue, cancellationToken);
        }

        async Task<AmqpConnection> OpenConnectionAsync(Uri addressUri, SaslHandler saslHandler, AmqpConnectionSettings connectionSettings, TimeSpan timeout, CancellationToken cancellationToken)
        {
            AmqpSettings settings = this.GetAmqpSettings(saslHandler);
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            TransportBase transport = await this.GetTransportAsync(addressUri, settings, timeoutHelper.RemainingTime(), cancellationToken).ConfigureAwait(false);

            try
            {
                if (connectionSettings == null)
                {
                    connectionSettings = new AmqpConnectionSettings();
                }

                connectionSettings.ContainerId = connectionSettings.ContainerId ?? Guid.NewGuid().ToString();
                connectionSettings.HostName = connectionSettings.HostName ?? addressUri.Host;

                return await this.CreateAndOpenConnectionAsync(transport, settings, connectionSettings, timeoutHelper.RemainingTime(), cancellationToken);
            }
            catch
            {
                transport.Abort();
                throw;
            }
        }

        /// <summary>
        /// Create and open the connection. Intended for internal use only.
        /// </summary>
        /// <param name="transport">The transport to be used for the connection.</param>
        /// <param name="settings">The AMQP settings to be used for the connection.</param>
        /// <param name="connectionSettings">The AmqpConnectionSettings to be used for the connection.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An AMQP connection.</returns>
        protected virtual async Task<AmqpConnection> CreateAndOpenConnectionAsync(
            TransportBase transport, 
            AmqpSettings settings, 
            AmqpConnectionSettings connectionSettings, 
            TimeSpan timeout, 
            CancellationToken cancellationToken)
        {
            AmqpConnection connection = new AmqpConnection(transport, settings, connectionSettings);
            await Task.Factory.FromAsync(
                static (t, k, c, s) => ((AmqpConnection)s).BeginOpen(t, k, c, s),
                static r => ((AmqpConnection)r.AsyncState).EndOpen(r),
                timeout,
                cancellationToken,
                connection)
                .ConfigureAwait(false);

            return connection;
        }

        internal AmqpSettings GetAmqpSettings(SaslHandler saslHandler)
        {
            AmqpSettings settings = this.settings.Clone();
            settings.TransportProviders.Clear();

            if (saslHandler != null)
            {
                // Provider for "AMQP3100"
                SaslTransportProvider saslProvider = new SaslTransportProvider(AmqpVersion.V100);
                saslProvider.AddHandler(saslHandler);
                settings.TransportProviders.Add(saslProvider);
            }

            // Provider for "AMQP0100"
            AmqpTransportProvider amqpProvider = new AmqpTransportProvider(AmqpVersion.V100);
            settings.TransportProviders.Add(amqpProvider);

            return settings;
        }

        internal Task<TransportBase> GetTransportAsync(Uri addressUri, AmqpSettings settings, TimeSpan timeout, CancellationToken cancellationToken)
        {
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

                // Provider first, then TLS settings
                var tls = new TlsTransportSettings(tcpSettings) { TargetHost = addressUri.Host };
                TlsTransportProvider tlsProvider = this.settings.GetTransportProvider<TlsTransportProvider>();
                if (tlsProvider != null)
                {
                    tls.CertificateValidationCallback = tlsProvider.Settings.CertificateValidationCallback;
                    tls.CheckCertificateRevocation = tlsProvider.Settings.CheckCertificateRevocation;
                    tls.Certificate = tlsProvider.Settings.Certificate;
                    tls.Protocols = tlsProvider.Settings.Protocols;
                }

                if (this.tlsSettings != null)
                {
                    tls.CertificateValidationCallback = this.tlsSettings.CertificateValidationCallback;
                    tls.CheckCertificateRevocation = this.tlsSettings.CheckCertificateRevocation;
                    tls.Certificate = this.tlsSettings.Certificate;
                    tls.Protocols = this.tlsSettings.Protocols;
                }

                transportSettings = tls;
            }
            else if (addressUri.Scheme.Equals(WebSocketTransportSettings.WebSockets, StringComparison.OrdinalIgnoreCase) ||
                addressUri.Scheme.Equals(WebSocketTransportSettings.SecureWebSockets, StringComparison.OrdinalIgnoreCase))
            {
                transportSettings = new WebSocketTransportSettings() { Uri = addressUri };
            }
            else
            {
                throw new NotSupportedException(addressUri.Scheme);
            }

            AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
            return initiator.ConnectAsync(timeout, cancellationToken);
        }

        static SaslHandler CreateDefaultSaslHandlerIfPossible(Uri addressUri)
        {
            SaslHandler saslHandler = null;

            if (!string.IsNullOrEmpty(addressUri.UserInfo))
            {
                string[] parts = addressUri.UserInfo.Split(':');
                if (parts.Length > 2)
                {
                    throw new ArgumentException("addressUri.UserInfo " + addressUri.UserInfo);
                }

                string userName = Uri.UnescapeDataString(parts[0]);
                string password = parts.Length > 1 ? Uri.UnescapeDataString(parts[1]) : string.Empty;

                saslHandler = new SaslPlainHandler() { AuthenticationIdentity = userName, Password = password };
            }

            return saslHandler;
        }
    }
}
