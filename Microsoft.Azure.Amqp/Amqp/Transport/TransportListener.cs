// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Threading;
    using Microsoft.Azure.Amqp;

    public abstract class TransportListener : AmqpObject
    {
        WaitCallback notifyAccept;
        Action<TransportListener, TransportAsyncCallbackArgs> acceptCallback;

        protected TransportListener(string type)
            : base(type)
        {
        }

        public void Listen(Action<TransportListener, TransportAsyncCallbackArgs> callback)
        {
            this.notifyAccept = this.NotifyAccept;
            this.acceptCallback = callback;

            this.OnListen();
        }

        protected override void OnOpen(TimeSpan timeout)
        {
            this.State = AmqpObjectState.Opened;
        }

        protected override void OnClose(TimeSpan timeout)
        {
            this.CloseInternal();
            this.State = AmqpObjectState.End;
        }

        protected override bool OpenInternal()
        {
            return true;
        }

        protected override bool CloseInternal()
        {
            return true;
        }

        protected override void AbortInternal()
        {
        }

        protected void OnTransportAccepted(TransportAsyncCallbackArgs args)
        {
            if (args.CompletedSynchronously)
            {
                ActionItem.Schedule(this.notifyAccept, args);
            }
            else
            {
                this.NotifyAccept(args);
            }
        }

        protected abstract void OnListen();

        void NotifyAccept(object state)
        {
            TransportAsyncCallbackArgs args = (TransportAsyncCallbackArgs)state;
            this.acceptCallback(this, args);
        }
    }
}
