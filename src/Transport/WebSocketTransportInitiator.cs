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
            cws.Options.SetBuffer(this.settings.ReceiveBufferSize, this.settings.SendBufferSize);
            if (this.settings.Proxy != null)
            {
                cws.Options.Proxy = this.settings.Proxy;
            }

            var task = new TimeoutTaskSource<ClientWebSocket>(
                cws,
                s => s.ConnectAsync(this.settings.Uri, CancellationToken.None),
                s => s.Abort(),
                timeout).Task;

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

        sealed class TimeoutTaskSource<T> : TaskCompletionSource<T> where T : class
        {
            readonly T t;
            readonly TimeSpan timeout;
            readonly ITimer timer;
            readonly Action<T> onTimeout;

            public TimeoutTaskSource(T t, Func<T, Task> onStart, Action<T> onTimeout, TimeSpan timeout)
            {
                this.t = t;
                this.onTimeout = onTimeout;
                this.timeout = timeout;
                this.timer = SystemTimerFactory.Default.Create(OnTimer, this, timeout);

                Task task = onStart(t);
                task.ContinueWith((_t, _s) => ((TimeoutTaskSource<T>)_s).OnTask(_t), this);
            }

            static void OnTimer(object state)
            {
                var thisPtr = (TimeoutTaskSource<T>)state;
                thisPtr.onTimeout(thisPtr.t);
                thisPtr.TrySetException(new TimeoutException(AmqpResources.GetString(AmqpResources.AmqpTimeout, thisPtr.timeout, typeof(T).Name)));
            }

            void OnTask(Task inner)
            {
                this.timer.Cancel();
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