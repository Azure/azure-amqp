// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Net.Sockets;
    using System.Threading;

    sealed class TcpTransportInitiator : TransportInitiator
    {
        readonly TcpTransportSettings transportSettings;
        TransportAsyncCallbackArgs callbackArgs;
        SocketAsyncEventArgs connectEventArgs;
        Timer timer;
        int state;

        internal TcpTransportInitiator(TcpTransportSettings transportSettings)
        {
            this.transportSettings = transportSettings;
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            this.callbackArgs = callbackArgs;
            this.callbackArgs.Exception = null;
            this.callbackArgs.Transport = null;

            DnsEndPoint dnsEndPoint = new DnsEndPoint(this.transportSettings.Host, this.transportSettings.Port);
            this.connectEventArgs = new SocketAsyncEventArgs();
            this.connectEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(OnConnectComplete);
            this.connectEventArgs.RemoteEndPoint = dnsEndPoint;
            this.connectEventArgs.UserToken = this;

            if (timeout < TimeSpan.MaxValue)
            {
                this.timer = new Timer(s => OnTimer(s), this, timeout, Timeout.InfiniteTimeSpan);
            }

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
                if (Interlocked.CompareExchange(ref this.state, 1, 0) == 0)
                {
                    this.Complete(connectEventArgs, true);
                }

                return false;
            }
        }

        static void OnConnectComplete(object sender, SocketAsyncEventArgs e)
        {
            TcpTransportInitiator thisPtr = (TcpTransportInitiator)e.UserToken;
            if (Interlocked.CompareExchange(ref thisPtr.state, 1, 0) == 0)
            {
                thisPtr.Complete(e, false);
            }
        }

        static void OnTimer(object obj)
        {
            var thisPtr = (TcpTransportInitiator)obj;
            if (Interlocked.CompareExchange(ref thisPtr.state, 1, 0) == 0)
            {
                thisPtr.connectEventArgs.SocketError = SocketError.TimedOut;
                thisPtr.Complete(thisPtr.connectEventArgs, false);
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
            this.timer?.Dispose();

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
