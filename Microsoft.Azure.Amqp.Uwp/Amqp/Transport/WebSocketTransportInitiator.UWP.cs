// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Networking.Sockets;

    sealed class WebSocketTransportInitiator : TransportInitiator
    {
        readonly WebSocketTransportSettings settings;

        internal WebSocketTransportInitiator(WebSocketTransportSettings settings)
        {
            this.settings = settings;
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            StreamWebSocket sws = new StreamWebSocket();
            sws.Control.SupportedProtocols.Add(this.settings.SubProtocol);

            var cts = new CancellationTokenSource(timeout);
            var task = sws.ConnectAsync(this.settings.Uri).AsTask(cts.Token);
            if (task.IsCompleted)
            {
                this.OnConnect(callbackArgs, task, sws, cts);
                return false;
            }

            task.ContinueWith(t =>
            {
                this.OnConnect(callbackArgs, t, sws, cts);
                callbackArgs.CompletedCallback(callbackArgs);
            });
            return true;
        }

        void OnConnect(TransportAsyncCallbackArgs callbackArgs, Task t, StreamWebSocket sws, CancellationTokenSource cts)
        {
            cts.Dispose();
            if (t.IsFaulted)
            {
                sws.Dispose();
                callbackArgs.Exception = t.Exception.InnerException;
            }
            else if (t.IsCanceled)
            {
                sws.Dispose();
                callbackArgs.Exception = new OperationCanceledException();
            }
            else
            {
                callbackArgs.Transport = new WebSocketTransport(sws, this.settings.Uri);
            }
        }
    }
}