// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using Windows.Networking;
    using Windows.Networking.Sockets;

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
            var streamSocket = new StreamSocket();
            var addr = this.transportSettings.Host;

            this.callbackArgs = callbackArgs;

            var connectTask = streamSocket.ConnectAsync(new HostName(addr), this.transportSettings.Port.ToString(), SocketProtectionLevel.PlainSocket).AsTask();
            if (connectTask.IsCompleted)
            {
                var transport = new TcpTransport(streamSocket, this.transportSettings);
                transport.Open();
                callbackArgs.CompletedSynchronously = true;
                callbackArgs.Transport = transport;
                return false;
            }

            connectTask.ContinueWith(_t =>
            {
                if (_t.IsFaulted)
                {
                    this.callbackArgs.Exception = _t.Exception.InnerException;
                }
                else if (_t.IsCanceled)
                {
                    this.callbackArgs.Exception = new OperationCanceledException();
                }
                else
                {
                    var transport = new TcpTransport(streamSocket, this.transportSettings);
                    transport.Open();
                    this.callbackArgs.CompletedSynchronously = false;
                    this.callbackArgs.Transport = transport;
                }

                this.callbackArgs.CompletedCallback(this.callbackArgs);
            });
            return true;
        }
    }
}
