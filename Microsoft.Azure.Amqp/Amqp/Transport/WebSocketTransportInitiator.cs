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
        readonly WebSocketTransportSettings settings;

        internal WebSocketTransportInitiator(WebSocketTransportSettings settings)
        {
            this.settings = settings;
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            ClientWebSocket cws = new ClientWebSocket();
            cws.Options.AddSubProtocol(this.settings.SubProtocol);
            if (this.settings.Proxy != null)
            {
                cws.Options.Proxy = this.settings.Proxy;
            }
#if NET45 || MONOANDROID
            cws.Options.SetBuffer(this.settings.ReceiveBufferSize, this.settings.SendBufferSize);
#endif

            var cts = new CancellationTokenSource(timeout);
            Task task = cws.ConnectAsync(this.settings.Uri, cts.Token);
            if (task.IsCompleted)
            {
                this.OnConnect(callbackArgs, task, cws, cts);
                return false;
            }

            task.ContinueWith(t =>
            {
                this.OnConnect(callbackArgs, t, cws, cts);
                callbackArgs.CompletedCallback(callbackArgs);
            });
            return true;
        }

        void OnConnect(TransportAsyncCallbackArgs callbackArgs, Task t, ClientWebSocket cws, CancellationTokenSource cts)
        {
            cts.Dispose();
            if (t.IsFaulted)
            {
                cws.Dispose();
                callbackArgs.Exception = t.Exception.InnerException;
            }
            else if (t.IsCanceled)
            {
                cws.Dispose();
                callbackArgs.Exception = new OperationCanceledException();
            }
            else
            {
                callbackArgs.Transport = new WebSocketTransport(cws, this.settings.Uri);
            }
        }
    }
}