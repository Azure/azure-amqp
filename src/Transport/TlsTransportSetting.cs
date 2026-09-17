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
        // SslProtocols.None to use system default is only supported on net47+, netstandard2.0+, and net5+.
        // When the app (not the lib) targets lower versions, using SslProtocols.None results in an argument error.
        // The lib cannot control the app's target framework, so we handle the argument error on lower platforms
        // and revert back to the legacy default SslProtocols to maintain backward compatibility.
        // https://learn.microsoft.com/en-us/dotnet/framework/network-programming/tls
#if NET10_0_OR_GREATER
        const SslProtocols LegacyDefaultSslProtocols = SslProtocols.Tls12;
#else
        const SslProtocols LegacyDefaultSslProtocols = SslProtocols.Tls | SslProtocols.Tls11 | SslProtocols.Tls12;
#endif
        static SslProtocols? systemSslProtocols;
        SslProtocols? userSslProtocols;

        /// <summary>
        /// The inner transport settings.
        /// </summary>
        public readonly TransportSettings innerSettings;

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
            get { return this.userSslProtocols ?? SslProtocols.None; }
            set { this.userSslProtocols = value; }
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

        internal SslProtocols? UserProtocols
        {
            get { return this.userSslProtocols; }
            set { this.userSslProtocols = value; }
        }

        internal SslProtocols InternalProtocols
        {
            get { return this.userSslProtocols ?? systemSslProtocols ?? SslProtocols.None; }
        }

        internal SslProtocols RefreshProtocolsOnArgumentError()
        {
            if (this.userSslProtocols == null && systemSslProtocols == null)
            {
                systemSslProtocols = LegacyDefaultSslProtocols;
            }

            return this.InternalProtocols;
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
