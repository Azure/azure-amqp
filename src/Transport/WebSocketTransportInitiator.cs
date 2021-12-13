// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
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
            if (this.settings.InternalSendBufferSize > 0 || this.settings.InternalReceiveBufferSize > 0)
            {
                cws.Options.SetBuffer(this.settings.ReceiveBufferSize, this.settings.SendBufferSize);
            }

            if (this.settings.Proxy != null)
            {
                cws.Options.Proxy = this.settings.Proxy;
            }

            var task = new TimeoutTaskSource<ClientWebSocket>(
                cws,
                s => s.ConnectAsync(this.settings.Uri, CancellationToken.None),
                static s => s.Abort(),
                timeout).Task;

            if (task.IsCompleted)
            {
                callbackArgs.Transport = new WebSocketTransport(cws, this.settings.Uri, null,
                    new DnsEndPoint(this.settings.Uri.Host, this.settings.Uri.Port));
                return false;
            }

            task.ContinueWith(static (t,s) =>
            {
                var (transport, callbackArgs, cws) = (Tuple<WebSocketTransportInitiator, TransportAsyncCallbackArgs, ClientWebSocket>) s;
                if (t.IsFaulted)
                {
                    callbackArgs.Exception = t.Exception?.InnerException;
                }
                else if (t.IsCanceled)
                {
                    callbackArgs.Exception = new OperationCanceledException();
                }
                else
                {
                    callbackArgs.Transport = new WebSocketTransport(cws, transport.settings.Uri, null,
                        new DnsEndPoint(transport.settings.Uri.Host, transport.settings.Uri.Port));
                }

                callbackArgs.CompletedCallback(callbackArgs);
            }, Tuple.Create(this, callbackArgs, cws));
            return true;
        }

        sealed class TimeoutTaskSource<T> : TaskCompletionSource<T> where T : class
        {
            readonly T t;
            readonly TimeSpan timeout;
            readonly Timer timer;
            readonly Action<T> onTimeout;

            public TimeoutTaskSource(T t, Func<T, Task> onStart, Action<T> onTimeout, TimeSpan timeout)
            {
                this.t = t;
                this.onTimeout = onTimeout;
                this.timeout = timeout;
                this.timer = new Timer(static s => OnTimer(s), this, timeout, Timeout.InfiniteTimeSpan);

                Task task = onStart(t);
                task.ContinueWith(static (_t, _s) => ((TimeoutTaskSource<T>)_s).OnTask(_t), this);
            }

            static void OnTimer(object state)
            {
                var thisPtr = (TimeoutTaskSource<T>)state;
                thisPtr.onTimeout(thisPtr.t);
                thisPtr.TrySetException(new TimeoutException(AmqpResources.GetString(AmqpResources.AmqpTimeout, thisPtr.timeout, typeof(T).Name)));
            }

            void OnTask(Task inner)
            {
                this.timer.Dispose();
                if (inner.IsFaulted)
                {
                    this.TrySetException(inner.Exception.InnerException);
                }
                else if (inner.IsCanceled)
                {
                    this.TrySetCanceled();
                }
                else
                {
                    this.TrySetResult(this.t);
                }
            }
        }
    }
}