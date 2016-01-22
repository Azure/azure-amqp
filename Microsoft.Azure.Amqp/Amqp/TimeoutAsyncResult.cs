// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    
    abstract class TimeoutAsyncResult<T> : AsyncResult where T : class
    {
        static readonly Action<object> timerCallback = new Action<object>(OnTimerCallback);
        readonly TimeSpan timeout;
        IOThreadTimer timer;
        int completed;
#if DEBUG
        bool setTimerCalled;  // make sure derived class always call SetTimer
#endif

        protected TimeoutAsyncResult(TimeSpan timeout, AsyncCallback callback, object state)
            : base(callback, state)
        {
            // The derived class must call SetTimer to start the timer.
            // Timer is not started here because it could fire before the
            // derived class ctor completes.
            this.timeout = timeout;
        }

        protected abstract T Target { get; }

        protected void SetTimer()
        {
#if DEBUG
            this.setTimerCalled = true;
#endif
            if (this.timeout != TimeSpan.MaxValue)
            {
                this.timer = new IOThreadTimer(timerCallback, this, true);
                this.timer.Set(this.timeout);
            }
        }

        protected virtual void CompleteOnTimer()
        {
            this.CompleteInternal(false, new TimeoutException(AmqpResources.GetString(AmqpResources.AmqpTimeout, this.timeout, this.Target)));
        }

        protected void CompleteSelf(bool syncComplete)
        {
            this.CompleteSelf(syncComplete, null);
        }

        protected void CompleteSelf(bool syncComplete, Exception exception)
        {
            if (this.timer != null)
            {
                this.timer.Cancel();
            }

            this.CompleteInternal(syncComplete, exception);
        }

        static void OnTimerCallback(object state)
        {
            TimeoutAsyncResult<T> thisPtr = (TimeoutAsyncResult<T>)state;
            thisPtr.CompleteOnTimer();
        }

        void CompleteInternal(bool syncComplete, Exception exception)
        {
#if DEBUG
            Fx.AssertAndThrow(this.setTimerCalled, "Must call SetTimer.");
#endif

#pragma warning disable 0420
            if (Interlocked.CompareExchange(ref this.completed, 1, 0) == 0)
#pragma warning restore 0420
            {
                if (exception == null)
                {
                    base.Complete(syncComplete);
                }
                else
                {
                    base.Complete(syncComplete, exception);
                }
            }
        }
    }
}
