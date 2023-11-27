// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
#if !PCL
    using System.Net.Security;
    using System.Security.Authentication;
    using System.Security.Cryptography.X509Certificates;
#endif

    public class TlsTransportSettings : TransportSettings
    {
#if !PCL
        // SslProtocols.None to use system default is only supported on net47+, netstandard2.0+, and net5+.
        // When the app (not the lib) targets lower versions, using SslProtocols.None results in an argument error.
        // The lib cannot control the app's target framework, so we handle the argument error on lower platforms
        // and revert back to the legacy default SslProtocols to maintain backward comptability.
        // Tls13 is supported on net48+, netcoreapp30+ (OpenSSL only?), and net5+.
        // https://learn.microsoft.com/en-us/dotnet/framework/network-programming/tls
        const SslProtocols LegacyDefaultSslProtocols = SslProtocols.Tls | SslProtocols.Tls11 | SslProtocols.Tls12;
        static SslProtocols? systemSslProtocols;
        SslProtocols? userSslProtocols;
#endif
        protected readonly TransportSettings innerSettings;

        public TlsTransportSettings()
            : this(null, true)
        {
            // Called to create a ssl upgrade transport setting. No inner settings is
            // required as the inner transport already exists for upgrading.
        }

        public TlsTransportSettings(TransportSettings innerSettings)
            : this(innerSettings, true)
        {
        }

        public TlsTransportSettings(TransportSettings innerSettings, bool isInitiator)
            : base()
        {
            this.innerSettings = innerSettings;
            this.IsInitiator = isInitiator;
            this.CheckCertificateRevocation = true;
        }

        public bool IsInitiator
        {
            get;
            set;
        }

        public string TargetHost
        {
            get;
            set;
        }

#if !PCL
        public SslProtocols Protocols
        {
            get { return this.userSslProtocols ?? SslProtocols.None; }
            set { this.userSslProtocols = value; }
        }

        public X509Certificate2 Certificate
        {
            get;
            set;
        }

        public RemoteCertificateValidationCallback CertificateValidationCallback
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
#endif

        public TransportSettings InnerTransportSettings
        {
            get { return this.innerSettings; }
        }

        public bool CheckCertificateRevocation
        {
            get;
            set;
        }

        public override TransportInitiator CreateInitiator()
        {
#if !PCL
            if (this.TargetHost == null)
            {
                throw new InvalidOperationException(CommonResources.TargetHostNotSet);
            }

            return new TlsTransportInitiator(this);
#else
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
#endif
        }

#if NET45 || NETSTANDARD || MONOANDROID
        public override TransportListener CreateListener()
        {
            if (this.Certificate == null)
            {
                throw new InvalidOperationException(CommonResources.ServerCertificateNotSet);
            }

            return new TlsTransportListener(this);
        }
#endif

        public override string ToString()
        {
            return this.innerSettings.ToString();
        }
    }
}
