// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// An AMQP link for sending messages.
    /// </summary>
    public sealed class SendingAmqpLink : AmqpLink, IWorkDelegate<AmqpMessage>
    {
        static readonly TimeSpan MinRequestCreditWindow = TimeSpan.FromSeconds(10);
        readonly SerializedWorker<AmqpMessage> pendingDeliveries;   // need link credit
        readonly WorkCollection<ArraySegment<byte>, SendAsyncResult, Outcome> inflightSends;
        Action<Delivery> dispositionListener;
        DateTime lastFlowRequestTime;
        ICollection<Delivery> deliveriesToBeResentUponRecovery;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="settings">The link settings.</param>
        public SendingAmqpLink(AmqpLinkSettings settings)
            : this(null, settings)
        {
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="session">The session where the link is created.</param>
        /// <param name="settings">The link settings.</param>
        public SendingAmqpLink(AmqpSession session, AmqpLinkSettings settings)
            : base("sender", session, settings)
        {
            // TODO: Add capability negotiation logic for BatchedMessageFormat to this.Settings
            this.pendingDeliveries = new SerializedWorker<AmqpMessage>(this);
            this.inflightSends = new WorkCollection<ArraySegment<byte>, SendAsyncResult, Outcome>(ByteArrayComparer.Instance);
            this.lastFlowRequestTime = DateTime.UtcNow;
        }

        /// <summary>
        /// Registers a disposition listener to handler delivery state changes.
        /// </summary>
        /// <param name="dispositionListener"></param>
        public void RegisterDispositionListener(Action<Delivery> dispositionListener)
        {
            if (Interlocked.Exchange(ref this.dispositionListener, dispositionListener) != null)
            {
                throw new InvalidOperationException(CommonResources.DispositionListenerAlreadyRegistered);
            }
        }

        /// <summary>
        /// Sends a message and does not wait for the disposition.
        /// </summary>
        /// <param name="message">The message to send.</param>
        /// <param name="deliveryTag">The delivery tag.</param>
        /// <param name="txnId">The transaction id.</param>
        public void SendMessageNoWait(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId)
        {
            this.SendMessageInternal(message, deliveryTag, txnId);
        }

        /// <summary>
        /// Starts an operation to send a message.
        /// </summary>
        /// <param name="message">The message to send.</param>
        /// <returns>A task that completes with the delivery outcome.</returns>
        public Task<Outcome> SendMessageAsync(AmqpMessage message)
        {
            return this.SendMessageAsync(message, CreateTag(), AmqpConstants.NullBinary, CancellationToken.None);
        }

        /// <summary>
        /// Starts an operation to send a message.
        /// </summary>
        /// <param name="message">The message to send.</param>
        /// <param name="deliveryTag">The delivery tag.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task that completes with the delivery outcome.</returns>
        public Task<Outcome> SendMessageAsync(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                static (p, t, k, c, s) => ((SendingAmqpLink)s).BeginSendMessage(p.Message, p.DeliveryTag, p.TxnId, t, k, c, s),
                static r => ((SendingAmqpLink)r.AsyncState).EndSendMessage(r),
                new SendMessageParam(message, deliveryTag, txnId),
                timeout,
                CancellationToken.None,
                this);
        }

        /// <summary>
        /// Starts an operation to send a message.
        /// </summary>
        /// <param name="message">The message to send.</param>
        /// <param name="deliveryTag">The delivery tag.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns></returns>
        public Task<Outcome> SendMessageAsync(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (p, t, k, c, s) => ((SendingAmqpLink)s).BeginSendMessage(p.Message, p.DeliveryTag, p.TxnId, t, k, c, s),
                static r => ((SendingAmqpLink)r.AsyncState).EndSendMessage(r),
                new SendMessageParam(message, deliveryTag, txnId),
                this.OperationTimeout,
                cancellationToken,
                this);
        }

        /// <summary>
        /// Begins an operation to send a message.
        /// </summary>
        /// <param name="message">The message to send.</param>
        /// <param name="deliveryTag">The delivery tag.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The object associated with the operation.</param>
        /// <returns>An <see cref="IAsyncResult"/>.</returns>
        public IAsyncResult BeginSendMessage(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginSendMessage(message, deliveryTag, txnId, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Ends the asynchronous send operation.
        /// </summary>
        /// <param name="result">The <see cref="IAsyncResult"/> returned by the begin method.</param>
        /// <returns>The delivery outcome.</returns>
        public Outcome EndSendMessage(IAsyncResult result)
        {
            return SendAsyncResult.End(result);
        }

        /// <summary>
        /// This method is not valid for a sending link.
        /// </summary>
        /// <param name="transfer"></param>
        /// <param name="delivery"></param>
        /// <returns></returns>
        protected override bool CreateDelivery(Transfer transfer, out Delivery delivery)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the number of deliveries that wait for link credits.
        /// </summary>
        public override uint Available
        {
            get
            {
                return (uint)this.pendingDeliveries.Count;
            }
        }

        /// <summary>
        /// Handles the state change of a delivery.
        /// </summary>
        /// <param name="delivery">The delivery whose state changed.</param>
        protected override void OnDisposeDeliveryInternal(Delivery delivery)
        {
            if (this.dispositionListener != null)
            {
                this.dispositionListener(delivery);
            }
            else
            {
                DeliveryState deliveryState = delivery.State;
                if (deliveryState.DescriptorCode == Received.Code)
                {
                    return;
                }

                TransactionalState txnState = deliveryState as TransactionalState;
                if (txnState != null)
                {
                    deliveryState = txnState.Outcome;
                }

                this.inflightSends.CompleteWork(delivery.DeliveryTag, false, (Outcome)deliveryState);
            }
        }

        /// <summary>
        /// This method is not valid for a sending link.
        /// </summary>
        /// <param name="delivery"></param>
        /// <param name="transfer"></param>
        /// <param name="frame"></param>
        protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame frame)
        {
            throw new AmqpException(AmqpErrorCode.NotAllowed, null);
        }

        /// <summary>
        /// Called when link credits are available.
        /// </summary>
        /// <param name="session">The session window.</param>
        /// <param name="link">The available link credits.</param>
        /// <param name="drain">true if the link is in drain mode, otherwise false.</param>
        /// <param name="txnId">The transaction id. It is ignored.</param>
        protected override void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId)
        {
            if (this.LinkCredit > 0)
            {
                this.pendingDeliveries.ContinueWork();
            }
        }

        /// <summary>
        /// Closes the link.
        /// </summary>
        /// <returns>true if the operation is completed, false otherwise.</returns>
        protected override bool CloseInternal()
        {
            this.AbortDeliveries();
            return base.CloseInternal();
        }

        /// <summary>
        /// Aborts the link.
        /// </summary>
        protected override void AbortInternal()
        {
            this.AbortDeliveries();
            base.AbortInternal();
        }

        /// <summary>
        /// Process and consolidate the unsettled deliveries sent with the remote Attach frame, by checking against the unsettled deliveries for this link terminus.
        /// </summary>
        /// <param name="remoteAttach">The incoming Attach from remote which contains the remote's unsettled delivery states.</param>
        protected override void ProcessUnsettledDeliveries(Attach remoteAttach)
        {
            if (this.Session.Connection.TerminusStore.TryGetLinkTerminusAsync(this.LinkIdentifier, out AmqpLinkTerminus linkTerminus).GetAwaiter().GetResult())
            {
                this.deliveriesToBeResentUponRecovery = Task.Run(() => linkTerminus.NegotiateUnsettledDeliveriesAsync(remoteAttach)).Result.Values;
                this.Opened += ResendDeliveriesOnOpen;
            }
        }

        static ArraySegment<byte> CreateTag()
        {
            return new ArraySegment<byte>(Guid.NewGuid().ToByteArray());
        }

        static void OnRequestCredit(object state)
        {
            try
            {
                SendingAmqpLink thisPtr = (SendingAmqpLink)state;
                if (thisPtr.State == AmqpObjectState.OpenSent ||
                    thisPtr.State == AmqpObjectState.Opened)
                {
                    thisPtr.SendFlow(true);
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
            }
        }

        internal IAsyncResult BeginSendMessage(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId,
            TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            this.ThrowIfClosed();
            if (this.dispositionListener != null)
            {
                throw new InvalidOperationException(CommonResources.DispositionListenerSetNotSupported);
            }

            message.ThrowIfDisposed();
            if (message.Link != null)
            {
                throw new InvalidOperationException(AmqpResources.AmqpCannotResendMessage);
            }

            if (message.Serialize(false) == 0)
            {
                throw new InvalidOperationException(AmqpResources.AmqpEmptyMessageNotAllowed);
            }

            return new SendAsyncResult(this, message, deliveryTag, txnId, timeout, cancellationToken, callback, state);
        }

        void AbortDeliveries()
        {
            if (this.pendingDeliveries != null)
            {
                this.pendingDeliveries.Abort();
            }

            if (this.inflightSends != null)
            {
                this.inflightSends.Abort();
            }
        }

        void SendMessageInternal(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId)
        {
            message.DeliveryTag = deliveryTag;
            message.Settled = this.Settings.SettleType == SettleMode.SettleOnSend;
            message.TxnId = txnId;
            this.pendingDeliveries.DoWork(message);
        }

        bool IWorkDelegate<AmqpMessage>.Invoke(AmqpMessage message)
        {
            DeliveryState deliveryState = message.State;
            if (deliveryState != null && deliveryState.DescriptorCode == Released.Code)
            {
                // message has been cancelled (e.g. timed out)
                return true;
            }

            bool success = this.TrySendDelivery(message);
            if (!success &&
                this.Session.State == AmqpObjectState.Opened &&
                DateTime.UtcNow - this.lastFlowRequestTime >= MinRequestCreditWindow)
            {
                // Tell the other side that we have some messages to send
                this.lastFlowRequestTime = DateTime.UtcNow;
                ActionItem.Schedule(static s => OnRequestCredit(s), this);
            }

            return success;
        }

        static void ResendDeliveriesOnOpen(object sender, EventArgs eventArgs)
        {
            var thisPtr = (SendingAmqpLink)sender;
            thisPtr.Opened -= ResendDeliveriesOnOpen;
            foreach (Delivery delivery in thisPtr.deliveriesToBeResentUponRecovery)
            {
                thisPtr.ForceSendDelivery(delivery);
            }

            thisPtr.deliveriesToBeResentUponRecovery = null;
        }

        readonly struct SendMessageParam
        {
            public SendMessageParam(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId)
            {
                this.Message = message;
                this.DeliveryTag = deliveryTag;
                this.TxnId = txnId;
            }

            public readonly AmqpMessage Message;
            public readonly ArraySegment<byte> DeliveryTag;
            public readonly ArraySegment<byte> TxnId;
        }

        sealed class SendAsyncResult : TimeoutAsyncResult<string>, IWork<Outcome>
        {
            readonly SendingAmqpLink link;
            readonly AmqpMessage message;
            readonly ArraySegment<byte> deliveryTag;
            readonly ArraySegment<byte> txnId;
            Outcome outcome;

            public SendAsyncResult(
                SendingAmqpLink link,
                AmqpMessage message,
                ArraySegment<byte> deliveryTag,
                ArraySegment<byte> txnId,
                TimeSpan timeout,
                CancellationToken cancellationToken,
                AsyncCallback callback,
                object state)
                : base(timeout, cancellationToken, callback, state)
            {
                this.link = link;
                this.message = message;
                this.deliveryTag = deliveryTag;
                this.txnId = txnId;
                this.link.inflightSends.StartWork(deliveryTag, this);
            }

            public static Outcome End(IAsyncResult result)
            {
                return AsyncResult.End<SendAsyncResult>(result).outcome;
            }

            public Outcome Outcome
            {
                get { return this.outcome; }
            }

            public void Start()
            {
                this.link.SendMessageInternal(this.message, this.deliveryTag, this.txnId);
                this.StartTracking();
            }

            public void Done(bool completedSynchronously, Outcome outcome)
            {
                this.outcome = outcome;
                this.CompleteSelf(completedSynchronously);
            }

            public override void Cancel(bool isSynchronous)
            {
                if (this.CancelInflight())
                {
                    this.CompleteSelf(isSynchronous, new TaskCanceledException());
                }
            }

            public void Cancel(bool completedSynchronously, Exception exception)
            {
                Exception completionException = exception;
                if (exception is OperationCanceledException &&
                    this.link.TerminalException != null)
                {
                    // the link was canceled because of some termination exception.
                    // Since the termination exception might not be caused
                    // by this specific async result, we should keep throwing
                    // OperationCancelledException (which is transient) but
                    // use the TerminalException message to allow user to
                    // debug.
                    completionException = new OperationCanceledException(this.link.TerminalException.Message, this.link.TerminalException);
                }

                this.CompleteSelf(completedSynchronously, completionException);
            }

            protected override string Target
            {
                get { return "message"; }
            }

            protected override void CompleteOnTimer()
            {
                this.CancelInflight();
                base.CompleteOnTimer();
            }

            bool CancelInflight()
            {
                if (this.link.inflightSends.TryRemoveWork(this.deliveryTag, out _))
                {
                    // try to remove this delivery on the other side, note that the message may
                    // still be sent and accepted by the broker already. This race is by design.
                    if (this.message.Link != null)
                    {
                        this.link.DisposeDelivery(this.message, true, AmqpConstants.ReleasedOutcome);
                    }
                    else
                    {
                        this.message.State = AmqpConstants.ReleasedOutcome;
                    }

                    return true;
                }

                return false;
            }
        }
    }
}
