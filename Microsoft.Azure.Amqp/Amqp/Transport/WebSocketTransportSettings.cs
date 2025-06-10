// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;

    public sealed class WebSocketTransportSettings : TransportSettings
    {
        internal const string WebSocketSubProtocol = "amqp";
        internal const string WebSockets = "ws";
        internal const string SecureWebSockets = "wss";
        internal const int WebSocketsPort = 80;
        internal const int SecureWebSocketsPort = 443;

        public WebSocketTransportSettings()
        {
            this.SendBufferSize = AmqpConstants.TransportBufferSize;
            this.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
            this.SubProtocol = WebSocketSubProtocol;
        }

        public Uri Uri
        {
            get;
            set;
        }

        public string SubProtocol
        {
            get;
            set;
        }

        public IWebProxy Proxy
        {
            get;
            set;
        }

        public override TransportInitiator CreateInitiator()
        {
            return new WebSocketTransportInitiator(this);
        }

#if NET45 || MONOANDROID
        public override TransportListener CreateListener()
        {
            return new WebSocketTransportListener(this.Uri.AbsoluteUri);
        }
#endif
#if NETSTANDARD || NET6_0_OR_GREATER
        public override TransportListener CreateListener()
        {
            throw new NotSupportedException();
        }
#endif
    }
}