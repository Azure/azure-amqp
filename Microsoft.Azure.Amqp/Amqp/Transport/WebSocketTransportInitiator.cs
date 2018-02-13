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

            Task task = cws.ConnectAsync(this.settings.Uri, CancellationToken.None).WithTimeout(timeout, () => "timeout");
            if (task.IsCompleted)
            {
                callbackArgs.Transport = new WebSocketTransport(cws, this.settings.Uri);
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
                    callbackArgs.Transport = new WebSocketTransport(cws, this.settings.Uri);
                }

                callbackArgs.CompletedCallback(callbackArgs);
            });
            return true;
        }
    }
}