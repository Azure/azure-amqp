// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
#if NET45
    using System;

    public sealed class WebSocketTransportSettings : TransportSettings
    {
        public WebSocketTransportSettings()
        {
            this.SendBufferSize = AmqpConstants.TransportBufferSize;
            this.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
            this.SubProtocol = WebSocketTransport.WebSocketSubProtocol;
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

        public override TransportInitiator CreateInitiator()
        {
            return new WebSocketTransportInitiator(this.Uri, this);
        }

        public override TransportListener CreateListener()
        {
            return new WebSocketTransportListener(this.Uri.AbsoluteUri);
        }
    }
#endif
}