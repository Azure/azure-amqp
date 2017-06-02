// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net.WebSockets;
    using System.Threading;
    using System.Threading.Tasks;

    sealed class WebSocketTransportInitiator : TransportInitiator
    {
        readonly Uri uri;
        readonly WebSocketTransportSettings webSocketTransportSettings;

        internal WebSocketTransportInitiator(Uri uri, WebSocketTransportSettings webSocketTransportSettings)
        {
            this.uri = uri;
            this.webSocketTransportSettings = webSocketTransportSettings;
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            ClientWebSocket cws = new ClientWebSocket();
            cws.Options.AddSubProtocol(this.webSocketTransportSettings.SubProtocol);
#if NET45
            cws.Options.SetBuffer(this.webSocketTransportSettings.ReceiveBufferSize, this.webSocketTransportSettings.SendBufferSize);
#endif

            Task task = cws.ConnectAsync(uri, CancellationToken.None).WithTimeout(timeout, () => "timeout");
            if (task.IsCompleted)
            {
                callbackArgs.Transport = new WebSocketTransport(cws, uri);
                return false;
            }

            task.ContinueWith(t =>
            {
                if (t.IsFaulted)
                {
                    callbackArgs.Exception = t.Exception.InnerException;
                }
                else if (t.IsCanceled)
                {
                    callbackArgs.Exception = new OperationCanceledException();
                }
                else
                {
                    callbackArgs.Transport = new WebSocketTransport(cws, uri);
                }

                callbackArgs.CompletedCallback(callbackArgs);
            });
            return true;
        }
    }
}