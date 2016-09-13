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
            DnsEndPoint dnsEndPoint = new DnsEndPoint(this.transportSettings.Host, this.transportSettings.Port);

            SocketAsyncEventArgs connectEventArgs = new SocketAsyncEventArgs();
            connectEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(OnConnectComplete);
            connectEventArgs.RemoteEndPoint = dnsEndPoint;
            connectEventArgs.UserToken = this;

#if MONOANDROID
            // Work around for Mono issue: https://github.com/rabbitmq/rabbitmq-dotnet-client/issues/171
            Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            bool connectResult = socket.ConnectAsync(connectEventArgs);
#else
            bool connectResult = Socket.ConnectAsync(SocketType.Stream, ProtocolType.Tcp, connectEventArgs);
#endif
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
            thisPtr.Complete(e, false);
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
                catch (Exception exp)
                {
                    if (Fx.IsFatal(exp))
                    {
                        throw;
                    }

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
