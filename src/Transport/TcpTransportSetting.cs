// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Globalization;

    /// <summary>
    /// Defines the TCP transport settings.
    /// </summary>
    public sealed class TcpTransportSettings : TransportSettings
    {
        const int DefaultTcpBacklog = 200;
        const int DefaultTcpAcceptorCount = 1;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public TcpTransportSettings()
            : base()
        {
            this.TcpBacklog = DefaultTcpBacklog;
            this.ListenerAcceptorCount = DefaultTcpAcceptorCount;
        }

        /// <summary>
        /// Gets or sets the network host.
        /// </summary>
        public string Host
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the TCP port.
        /// </summary>
        public int Port
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the TCP listener socket's backlog.
        /// </summary>
        public int TcpBacklog 
        { 
            get; 
            set; 
        }

        /// <summary>
        /// Creates a TCP transport initiator.
        /// </summary>
        /// <returns>The TCP transport initiator.</returns>
        public override TransportInitiator CreateInitiator()
        {
            return new TcpTransportInitiator(this);
        }

        /// <summary>
        /// Creates a TCP transport listener.
        /// </summary>
        /// <returns>The TCP transport listener.</returns>
        public override TransportListener CreateListener()
        {
            return new TcpTransportListener(this);
        }

        /// <summary>
        /// Returns a string representation of the object.
        /// </summary>
        /// <returns>A string representation of the object.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}:{1}", this.Host, this.Port);
        }
    }
}
