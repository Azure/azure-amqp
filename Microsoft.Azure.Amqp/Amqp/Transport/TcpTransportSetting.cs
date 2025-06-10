// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Globalization;

    public sealed class TcpTransportSettings : TransportSettings
    {
        const int DefaultTcpBacklog = 200;
        const int DefaultTcpAcceptorCount = 1;

        public TcpTransportSettings()
            : base()
        {
            this.TcpBacklog = DefaultTcpBacklog;
            this.ListenerAcceptorCount = DefaultTcpAcceptorCount;
        }

        public string Host
        {
            get;
            set;
        }

        public int Port
        {
            get;
            set;
        }

        public int TcpBacklog 
        { 
            get; 
            set; 
        }

        public override TransportInitiator CreateInitiator()
        {
#if !PCL
            return new TcpTransportInitiator(this);
#else
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
#endif
        }

#if NET45 || NETSTANDARD || MONOANDROID || NET6_0_OR_GREATER
        public override TransportListener CreateListener()
        {
            return new TcpTransportListener(this);
        }
#endif

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}:{1}", this.Host, this.Port);
        }
    }
}
