// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Encoding;
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
        readonly List<Delivery> unsettledDeliveriesToBeResent; // deliveries to be sent upon opening this link (for link recovery)
        Action<Delivery> dispositionListener;
        DateTime lastFlowRequestTime;

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
            this.unsettledDeliveriesToBeResent = new List<Delivery>();
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
        /// Opens the link.
        /// </summary>
        /// <returns>True if open is completed.</returns>
        protected override bool OpenInternal()
        {
            bool syncComplete = base.OpenInternal();
            if (this.Session.Connection.Settings.EnableLinkRecovery && this.State == AmqpObjectState.Opened)
            {
                this.ResendUnsettledDeliveriesUponOpen();
            }

            return syncComplete;
        }

        /// <summary>
        /// Decide on which unsettled deliveries should be resent to the remote, based on the remote delivery states.
        /// </summary>
        protected override void OnReceiveRemoteUnsettledDeliveries(Attach attach)
        {
            if (this.Session.Connection.Settings.EnableLinkRecovery && this.Terminus.Settings.Unsettled != null)
            {
                Fx.Assert(this.Terminus != null, "If link recovery is enabled, the link terminus should not be null.");
                foreach (KeyValuePair<MapKey, object> pair in this.Terminus.Settings.Unsettled)
                {
                    var deliveryTagMapKey = pair.Key;
                    ArraySegment<byte> deliveryTag = (ArraySegment<byte>)deliveryTagMapKey.Key;
                    var localDeliveryState = pair.Value as DeliveryState;
                    DeliveryState peerDeliveryState = null;
                    bool peerHasDelivery = attach.Unsettled?.TryGetValue(deliveryTagMapKey, out peerDeliveryState) == true;
                    bool remoteReachedTerminal = peerDeliveryState is Outcome;
                    bool alreadySettledOnSend = this.Settings.SettleType == SettleMode.SettleOnSend && (!peerHasDelivery || (localDeliveryState == null && peerDeliveryState == null));

                    Delivery unsettledDeliveryToBeResent = null;
                    if (localDeliveryState == null || localDeliveryState is Received)
                    {
                        // remoteReachedTerminal: OASIS AMQP doc section 3.4.6, delivery 3, 8
                        // alreadySettledOnSend: OASIS AMQP doc section 3.4.6, delivery 1, 4, 5
                        if (!remoteReachedTerminal && !alreadySettledOnSend)
                        {
                            // should ideally always be true, Terminus.Settings.Unsettled should be consistent with Terminus.UnsettledMap.
                            if (this.Terminus.UnsettledMap.TryGetValue(deliveryTag, out unsettledDeliveryToBeResent))
                            {
                                // we don't support resume sending partial payload with Received state.
                                unsettledDeliveryToBeResent.State = null;

                                // abort the delivery when the remote receiver somehow has it transactional
                                // or this sender has only delivered partially, OASIS AMQP doc section 3.4.6, delivery 6, 7, 9.
                                unsettledDeliveryToBeResent.Aborted = peerDeliveryState is TransactionalState || (localDeliveryState is Received && peerHasDelivery);
                            }
                        }
                    }
                    else if (localDeliveryState is Outcome)
                    {
                        // should ideally always be true, Terminus.Settings.Unsettled should be consistent with Terminus.UnsettledMap.
                        if (peerHasDelivery && this.Terminus.UnsettledMap.TryGetValue(deliveryTag, out unsettledDeliveryToBeResent))
                        {
                            if (remoteReachedTerminal)
                            {
                                // OASIS AMQP doc section 3.4.6, delivery 12, 13.
                                unsettledDeliveryToBeResent.Settled = peerDeliveryState.GetType() == localDeliveryState.GetType();
                            }
                            else
                            {
                                // OASIS AMQP doc section 3.4.6, delivery 11, 14, or when remote receiver is Transactional
                                unsettledDeliveryToBeResent.Aborted = true;
                            }
                        }
                        else
                        {
                            // OASIS AMQP doc section 3.4.6, delivery 10. Do nothing.
                        }
                    }
                    else if (localDeliveryState is TransactionalState localTransactionalState)
                    {
                        if (this.Terminus.UnsettledMap.TryGetValue(deliveryTag, out unsettledDeliveryToBeResent))
                        {
                            if (localTransactionalState.Outcome != null)
                            {
                                if (!peerHasDelivery)
                                {
                                    // no need to resend the delivery if local sender has reached terminal state and remote receiver does not have this deliery, similar to OASIS AMQP doc section 3.4.6, delivery 10.
                                    unsettledDeliveryToBeResent = null;
                                }
                                else if (peerDeliveryState is TransactionalState peerTransactionalState && peerTransactionalState.Outcome != null)
                                {
                                    // both the sender and receiver reached transactional terminal state, similar to OASIS AMQP doc section 3.4.6, delivery 12, 13.
                                    unsettledDeliveryToBeResent.Settled = peerTransactionalState.Outcome.GetType() == localTransactionalState.Outcome.GetType();
                                }
                                else
                                {
                                    // the receiver side is not in a transactional state, which should not happen because the sender is in transactional state.
                                    // OR, the receiver side is also in a transactional state but has not reached terminal outcome, similar to OASIS AMQP doc section 3.4.6, delivery 11, 14.
                                    unsettledDeliveryToBeResent.Aborted = true;
                                }
                            }
                            else
                            {
                                if (!peerHasDelivery)
                                {
                                    // The receiver does not have any record of the delivery, similar to OASIS AMQP doc section 3.4.6, delivery 1.
                                    if (alreadySettledOnSend)
                                    {
                                        unsettledDeliveryToBeResent = null;
                                    }
                                }
                                else if (peerDeliveryState is TransactionalState peerTransactionalState && peerTransactionalState.Outcome != null)
                                {
                                    // the receiver has already reached terminal oucome, no need to resend, similar to OASIS AMQP doc section 3.4.6, delivery 3, 8
                                    unsettledDeliveryToBeResent = null;
                                }
                                else
                                {
                                    // the receiver has not reached terminal outcome and may or may not have received the whole delivery. Abort to be safe.
                                    unsettledDeliveryToBeResent.Aborted = true;
                                }
                            }
                        }
                    }

                    if (unsettledDeliveryToBeResent != null)
                    {
                        unsettledDeliveryToBeResent.Resume = peerHasDelivery;
                        this.unsettledDeliveriesToBeResent.Add(unsettledDeliveryToBeResent);
                    }
                }

                if (this.Session.Connection.Settings.EnableLinkRecovery && this.State == AmqpObjectState.Opened)
                {
                    this.ResendUnsettledDeliveriesUponOpen();
                }
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
                ActionItem.Schedule(s => OnRequestCredit(s), this);
            }

            return success;
        }

        void ResendUnsettledDeliveriesUponOpen()
        {
            foreach (Delivery delivery in this.unsettledDeliveriesToBeResent)
            {
                this.StartSendDelivery(delivery);
            }

            this.unsettledDeliveriesToBeResent.Clear();
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

            public override void Cancel()
            {
                this.CancelInflight();
                this.CompleteSelf(false, new TaskCanceledException());
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

            void CancelInflight()
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
                }
            }
        }
    }
}
