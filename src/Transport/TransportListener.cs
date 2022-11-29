// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Threading;
    using Microsoft.Azure.Amqp;

    /// <summary>
    /// Defines the base class of transport listeners.
    /// </summary>
    public abstract class TransportListener : AmqpObject
    {
        WaitCallback notifyAccept;
        Action<TransportListener, TransportAsyncCallbackArgs> acceptCallback;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="type">A prefix to the object name for debugging purposes.</param>
        protected TransportListener(string type)
            : base(type)
        {
        }

        /// <summary>
        /// Listens for incoming transports.
        /// </summary>
        /// <param name="callback">The callback to invoke when a transport is accepted.</param>
        public void Listen(Action<TransportListener, TransportAsyncCallbackArgs> callback)
        {
            this.notifyAccept = this.NotifyAccept;
            this.acceptCallback = callback;

            this.OnListen();
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="timeout">The timeout.</param>
        protected override void OnOpen(TimeSpan timeout)
        {
            this.State = AmqpObjectState.Opened;
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <param name="timeout">The timeout.</param>
        protected override void OnClose(TimeSpan timeout)
        {
            this.CloseInternal();
            this.State = AmqpObjectState.End;
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <returns>true.</returns>
        protected override bool OpenInternal()
        {
            return true;
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>true.</returns>
        protected override bool CloseInternal()
        {
            return true;
        }

        /// <summary>
        /// Aborts the object.
        /// </summary>
        protected override void AbortInternal()
        {
        }

        /// <summary>
        /// Called when a transport is accepted.
        /// </summary>
        /// <param name="args">The <see cref="TransportAsyncCallbackArgs"/>.</param>
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

        /// <summary>
        /// When overriden in derived classes, starts the listen operation.
        /// </summary>
        protected abstract void OnListen();

        void NotifyAccept(object state)
        {
            TransportAsyncCallbackArgs args = (TransportAsyncCallbackArgs)state;
            this.acceptCallback(this, args);
        }
    }
}
