// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#if WINDOWS_UWP
namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;

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
            var streamSocket = new Windows.Networking.Sockets.StreamSocket();
            var addr = this.transportSettings.Host;

            this.callbackArgs = callbackArgs;

            var connectTask = streamSocket.ConnectAsync(new Windows.Networking.HostName(addr), this.transportSettings.Port.ToString(), Windows.Networking.Sockets.SocketProtectionLevel.PlainSocket).AsTask();
            connectTask.ContinueWith(_ =>
            {
                TransportBase transport = null;
                Exception exception = null;

                try
                {
                    transport = new TcpTransport(streamSocket, this.transportSettings);
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

                var completeSynchronously = false;
                this.callbackArgs.CompletedSynchronously = completeSynchronously;
                this.callbackArgs.Exception = exception;
                this.callbackArgs.Transport = transport;

                if (!completeSynchronously)
                {
                    this.callbackArgs.CompletedCallback(this.callbackArgs);
                }
            });
            return true;
        }
    }
}
#endif
