// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net.Security;
    using System.Security.Authentication;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// Defines the TLS transport settings.
    /// </summary>
    public class TlsTransportSettings : TransportSettings
    {
        const SslProtocols DefaultSslProtocols = SslProtocols.Tls | SslProtocols.Tls11 | SslProtocols.Tls12;
        readonly TransportSettings innerSettings;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public TlsTransportSettings()
            : this(null, true)
        {
            // Called to create a ssl upgrade transport setting. No inner settings is
            // required as the inner transport already exists for upgrading.
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="innerSettings">The inner transport settings.</param>
        public TlsTransportSettings(TransportSettings innerSettings)
            : this(innerSettings, true)
        {
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="innerSettings">The inner transport settings.</param>
        /// <param name="isInitiator">true if it is for the initiator, false otherwise.</param>
        public TlsTransportSettings(TransportSettings innerSettings, bool isInitiator)
            : base()
        {
            this.innerSettings = innerSettings;
            this.IsInitiator = isInitiator;
            this.Protocols = DefaultSslProtocols;
            this.CheckCertificateRevocation = true;
        }

        /// <summary>
        /// Gets the inner transport settings.
        /// </summary>
        public TransportSettings InnerTransportSettings => this.innerSettings;

        /// <summary>
        /// Gets or sets the initiator flag.
        /// </summary>
        public bool IsInitiator
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the target host of the TLS transport.
        /// </summary>
        public string TargetHost
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the <see cref="SslProtocols"/> to be used.
        /// </summary>
        public SslProtocols Protocols
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a certificate. For initiator it is the client
        /// authentication certificate; for lister, it is the service
        /// certificate.
        /// </summary>
        public X509Certificate2 Certificate
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the remote certificate validation callback.
        /// </summary>
        public RemoteCertificateValidationCallback CertificateValidationCallback
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the check certificate revocation flag.
        /// </summary>
        public bool CheckCertificateRevocation
        {
            get;
            set;
        }

        /// <summary>
        /// Creates a transport initiator.
        /// </summary>
        /// <returns>A <see cref="TlsTransportInitiator"/>.</returns>
        public override TransportInitiator CreateInitiator()
        {
            if (this.TargetHost == null)
            {
                throw new InvalidOperationException(CommonResources.TargetHostNotSet);
            }

            return new TlsTransportInitiator(this);
        }

        /// <summary>
        /// Creates a transport listener.
        /// </summary>
        /// <returns>A <see cref="TlsTransportListener"/>.</returns>
        public override TransportListener CreateListener()
        {
            if (this.Certificate == null)
            {
                throw new InvalidOperationException(CommonResources.ServerCertificateNotSet);
            }

            return new TlsTransportListener(this);
        }

        /// <summary>
        /// Gets a string representation of the object.
        /// </summary>
        /// <returns>A string representation of the object.</returns>
        public override string ToString()
        {
            return this.innerSettings.ToString();
        }
    }
}
