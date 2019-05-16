// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;

    /// <summary>
    /// Defines the web socket transport settings.
    /// </summary>
    public sealed class WebSocketTransportSettings : TransportSettings
    {
        internal const string WebSocketSubProtocol = "amqp";
        internal const string WebSockets = "ws";
        internal const string SecureWebSockets = "wss";
        internal const int WebSocketsPort = 80;
        internal const int SecureWebSocketsPort = 443;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public WebSocketTransportSettings()
        {
            this.SendBufferSize = AmqpConstants.TransportBufferSize;
            this.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
            this.SubProtocol = WebSocketSubProtocol;
        }

        /// <summary>
        /// Gets or sets the Uri of the web socket endpoint.
        /// </summary>
        public Uri Uri
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the subprotocol.
        /// </summary>
        public string SubProtocol
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the <see cref="IWebProxy"/>.
        /// </summary>
        public IWebProxy Proxy
        {
            get;
            set;
        }

        /// <summary>
        /// Creates a transport initiator.
        /// </summary>
        /// <returns>The transport initiator.</returns>
        public override TransportInitiator CreateInitiator()
        {
            return new WebSocketTransportInitiator(this);
        }

        /// <summary>
        /// Creates a transport listener.
        /// </summary>
        /// <returns>The transport listener.</returns>
        public override TransportListener CreateListener()
        {
            return new WebSocketTransportListener(this.Uri.AbsoluteUri);
        }
    }
}