// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
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

            var task = sws.ConnectAsync(this.settings.Uri).AsTask().WithTimeout(timeout, () => "timeout");
            if (task.IsCompleted)
            {
                callbackArgs.Transport = new WebSocketTransport(sws, this.settings.Uri);
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
                    callbackArgs.Transport = new WebSocketTransport(sws, this.settings.Uri);
                }

                callbackArgs.CompletedCallback(callbackArgs);
            });
            return true;
        }
    }
}