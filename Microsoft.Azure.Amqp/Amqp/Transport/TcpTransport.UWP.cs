// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Net;
    using Windows.Networking.Sockets;

    [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
        Justification = "Uses custom scheme for cleanup")]
    sealed class TcpTransport : TransportBase
    {
        StreamSocket socket;

        public TcpTransport(Windows.Networking.Sockets.StreamSocket socket, TcpTransportSettings transportSettings)
            : base("tcp")
        {
            this.socket = socket;
        }

        public Windows.Networking.Sockets.StreamSocket Socket
        {
            get { return this.socket; }
        }

        public override EndPoint LocalEndPoint
        {
            get
            {
                throw new NotImplementedException("Not supported in UWP");
            }
        }

        public override EndPoint RemoteEndPoint
        {
            get
            {
                throw new NotImplementedException("Not supported in UWP");
            }
        }

        public sealed override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            throw new NotImplementedException("Not supported in UWP");
        }

        public sealed override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            throw new NotImplementedException("Not supported in UWP");
        }

        protected override bool CloseInternal()
        {
            this.socket.Dispose();
            return true;
        }

        protected override void AbortInternal()
        {
            this.socket.Dispose();
        }
    }
}
