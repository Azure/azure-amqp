// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Net.Sockets;

    sealed class TcpTransportInitiator : TransportInitiator
    {
        readonly TcpTransportSettings transportSettings;
        TransportAsyncCallbackArgs callbackArgs;

        internal TcpTransportInitiator(TcpTransportSettings transportSettings)
        {
            this.transportSettings = transportSettings;
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            // TODO: set socket connect timeout to timeout
            this.callbackArgs = callbackArgs;
            this.callbackArgs.Exception = null;
            this.callbackArgs.Transport = null;
            DnsEndPoint dnsEndPoint = new DnsEndPoint(this.transportSettings.Host, this.transportSettings.Port);

            SocketAsyncEventArgs connectEventArgs = new SocketAsyncEventArgs();
            connectEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(OnConnectComplete);
            connectEventArgs.RemoteEndPoint = dnsEndPoint;
            connectEventArgs.UserToken = this;

            // On Linux platform, socket connections are allowed to be initiated on the socket instance 
            // with hostname due to multiple IP address DNS resolution possibility.
            // They suggest either using static Connect API or IP address directly.
            bool connectResult = Socket.ConnectAsync(SocketType.Stream, ProtocolType.Tcp, connectEventArgs);
            if (connectResult)
            {
                return true;
            }
            else
            {
                this.Complete(connectEventArgs, true);
                return false;
            }
        }

        static void OnConnectComplete(object sender, SocketAsyncEventArgs e)
        {
            TcpTransportInitiator thisPtr = (TcpTransportInitiator)e.UserToken;
            if (thisPtr.callbackArgs.Transport == null && thisPtr.callbackArgs.Exception == null)
            {
                // Mono invokes the callback twice from the callback event handler
                // Ignore the second one as a workaround.
                thisPtr.Complete(e, false);
            }
        }

        void Complete(SocketAsyncEventArgs e, bool completeSynchronously)
        {
            TransportBase transport = null;
            Exception exception = null;
            if (e.SocketError != SocketError.Success)
            {
                exception = new SocketException((int)e.SocketError);
                if (e.AcceptSocket != null)
                {
                    e.AcceptSocket.Dispose();
                }
            }
            else
            {
                try
                {
                    Fx.Assert(e.ConnectSocket != null, "Must have a valid socket accepted.");
                    e.ConnectSocket.NoDelay = true;
                    transport = new TcpTransport(e.ConnectSocket, this.transportSettings);
                    transport.Open();
                }
                catch (Exception exp) when (!Fx.IsFatal(exp))
                {
                    exception = exp;
                    if (transport != null)
                    {
                        transport.SafeClose();
                    }
                    transport = null;
                }
            }

            e.Dispose();
            this.callbackArgs.CompletedSynchronously = completeSynchronously;
            this.callbackArgs.Exception = exception;
            this.callbackArgs.Transport = transport;

            if (!completeSynchronously)
            {
                this.callbackArgs.CompletedCallback(this.callbackArgs);
            }
        }
    }
}
