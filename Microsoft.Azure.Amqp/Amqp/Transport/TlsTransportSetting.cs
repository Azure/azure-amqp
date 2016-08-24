// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
#if !PCL
    using System.Net.Security;
#endif
#if !WINDOWS_UWP && !PCL
    using System.Security.Cryptography.X509Certificates;
#endif
    public sealed class TlsTransportSettings : TransportSettings
    {
        readonly TransportSettings innerSettings;

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
#if !WINDOWS_UWP
            this.CheckCertificateRevocation = true;
#endif
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

#if !WINDOWS_UWP && !PCL
        public X509Certificate2 Certificate
        {
            get;
            set;
        }
#endif

        public TransportSettings InnerTransportSettings
        {
            get { return this.innerSettings; }
        }

#if !WINDOWS_UWP
#if !PCL
        public RemoteCertificateValidationCallback CertificateValidationCallback
        {
            get;
            set;
        }
#endif
        public bool CheckCertificateRevocation
        {
            get;
            set;
        }
#endif
        public override TransportInitiator CreateInitiator()
        {
#if !PCL
            if (this.TargetHost == null)
            {
                throw new InvalidOperationException(CommonResources.TargetHostNotSet);
            }

            return new TlsTransportInitiator(this);
#else
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyMessage);
#endif
        }

#if !WINDOWS_UWP && !PCL
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
