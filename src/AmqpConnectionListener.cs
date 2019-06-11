// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// A listener to accept AMQP connections.
    /// </summary>
    public class AmqpConnectionListener
    {
        readonly AmqpTransportListener listener;
        readonly AmqpConnectionSettings connectionSettings;
        readonly IRuntimeProvider runtime;
        readonly HashSet<AmqpConnection> connections;
        readonly EventHandler onConnectionClosed;

        /// <summary>
        /// Initializes a listener.
        /// </summary>
        /// <param name="address">The address to listen on.</param>
        /// <param name="runtime">The provider to handle runtime operations.</param>
        public AmqpConnectionListener(string address, IRuntimeProvider runtime)
            : this(new[] { address }, new AmqpSettings() { RuntimeProvider = runtime }, new AmqpConnectionSettings())
        {
        }

        /// <summary>
        /// Initializes a listener.
        /// </summary>
        /// <param name="addresses">The addresses to listen on.</param>
        /// <param name="settings">The protocol settings of the listener.</param>
        /// <param name="connectionSettings">The connection settings applied to accepted connections.</param>
        public AmqpConnectionListener(IEnumerable<string> addresses, AmqpSettings settings, AmqpConnectionSettings connectionSettings)
        {
            if (settings.RuntimeProvider == null)
            {
                throw new ArgumentNullException("IRuntimeProvider");
            }

            this.runtime = settings.RuntimeProvider;
            this.connectionSettings = connectionSettings;
            this.connections = new HashSet<AmqpConnection>();
            this.onConnectionClosed = this.OnConnectionClosed;
            this.listener = CreateListener(addresses, settings);
        }

        /// <summary>
        /// Opens the listener.
        /// </summary>
        public void Open()
        {
            this.listener.Open();
            this.listener.Listen(this.OnAcceptTransport);
        }

        /// <summary>
        /// Closes the listener.
        /// </summary>
        public void Close()
        {
            this.listener.Close();
            List<AmqpConnection> snapshot = new List<AmqpConnection>(this.connections.Count);
            lock (this.connections)
            {
                snapshot.AddRange(this.connections);
                this.connections.Clear();
            }

            foreach (AmqpConnection connection in snapshot)
            {
                connection.Closed -= this.onConnectionClosed;
                connection.SafeClose();
            }
        }

        static AmqpTransportListener CreateListener(IEnumerable<string> addresses, AmqpSettings settings)
        {
            List<TransportListener> listeners = new List<TransportListener>();
            foreach (string address in addresses)
            {
                Uri uri = new Uri(address);
                bool isAmqps = string.Equals(AmqpConstants.SchemeAmqps, uri.Scheme, StringComparison.OrdinalIgnoreCase);
                TcpTransportSettings tcpSettings = new TcpTransportSettings()
                {
                    Host = uri.Host,
                    Port = uri.Port < 0 ? (isAmqps ? AmqpConstants.DefaultSecurePort : AmqpConstants.DefaultPort) : uri.Port
                };

                TransportListener tpListener;
                if (isAmqps)
                {
                    var tlsProvider = settings.GetTransportProvider<TlsTransportProvider>();
                    if (tlsProvider == null)
                    {
                        throw new InvalidOperationException($"TlsTransportProvider must be in AmqpSettings.TransportProviders for '{address}'.");
                    }

                    if (tlsProvider.Settings.Certificate == null)
                    {
                        throw new InvalidOperationException($"A server certificate is required in TlsTransportProvider for '{address}'.");
                    }

                    TlsTransportSettings tlsSettings = new TlsTransportSettings(tcpSettings, false);
                    tlsSettings.Certificate = tlsProvider.Settings.Certificate;
                    tlsSettings.CertificateValidationCallback = tlsProvider.Settings.CertificateValidationCallback;
                    tlsSettings.CheckCertificateRevocation = tlsProvider.Settings.CheckCertificateRevocation;

                    tpListener = tlsSettings.CreateListener();
                }
                else
                {
                    tpListener = tcpSettings.CreateListener();
                }

                listeners.Add(tpListener);
            }

            if (settings.GetTransportProvider<AmqpTransportProvider>() == null)
            {
                var amqpProvider = new AmqpTransportProvider(AmqpVersion.V100);
                settings.TransportProviders.Add(amqpProvider);
            }

            return new AmqpTransportListener(listeners, settings);
        }

        static void OnConnectionOpenComplete(IAsyncResult result)
        {
            var tuple = (Tuple<AmqpConnectionListener, TransportListener, AmqpConnection>)result.AsyncState;
            AmqpConnectionListener thisPtr = tuple.Item1;
            TransportListener innerListener = tuple.Item2;
            AmqpConnection connection = tuple.Item3;

            try
            {
                connection.EndOpen(result);
                lock (thisPtr.connections)
                {
                    thisPtr.connections.Add(connection);
                }

                connection.SafeAddClosed(thisPtr.onConnectionClosed);
            }
            catch (Exception ex) when (!Fx.IsFatal(ex))
            {
                AmqpTrace.Provider.AmqpLogError(connection, "EndOpen", ex);

                connection.SafeClose(ex);
            }
        }

        void OnConnectionClosed(object sender, EventArgs args)
        {
            AmqpConnection connection = (AmqpConnection)sender;
            lock (this.connections)
            {
                this.connections.Remove(connection);
            }
        }

        void OnAcceptTransport(TransportListener innerListener, TransportAsyncCallbackArgs args)
        {
            TransportBase transport = args.Transport;
            AmqpConnection connection = null;
            string operation = "Create";

            try
            {
                AmqpSettings amqpSettings = this.listener.AmqpSettings;  // no need to clone
                ProtocolHeader header = (ProtocolHeader)args.UserToken;
                AmqpConnectionSettings settings = this.connectionSettings.Clone();
                connection = this.runtime.CreateConnection(transport, header, false, this.listener.AmqpSettings, settings);

                operation = "BeginOpen";
                connection.BeginOpen(AmqpConstants.DefaultTimeout, OnConnectionOpenComplete, Tuple.Create(this, innerListener, connection));
            }
            catch (Exception ex) when (!Fx.IsFatal(ex))
            {
                AmqpTrace.Provider.AmqpLogError(innerListener, operation, ex);

                if (connection != null)
                {
                    connection.SafeClose(ex);
                }
                else
                {
                    transport.Abort();
                }
            }
        }
    }
}
