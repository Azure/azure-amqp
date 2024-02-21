// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    public sealed class SendingAmqpLink : AmqpLink, IWorkDelegate<AmqpMessage>
    {
        static readonly TimeSpan MinRequestCreditWindow = TimeSpan.FromSeconds(10);
        readonly SerializedWorker<AmqpMessage> pendingDeliveries;   // need link credit
        readonly WorkCollection<ArraySegment<byte>, SendAsyncResult, Outcome> inflightSends;
        Action<Delivery> dispositionListener;
        Action<uint, bool, ArraySegment<byte>> creditListener;
        DateTime lastFlowRequestTime;

        public SendingAmqpLink(AmqpLinkSettings settings)
            : this(null, settings)
        {
        }

        public SendingAmqpLink(AmqpSession session, AmqpLinkSettings settings)
            : base("sender", session, settings)
        {
            // TODO: Add capability negotiation logic for BatchedMessageFormat to this.Settings
            this.pendingDeliveries = new SerializedWorker<AmqpMessage>(this);
            this.inflightSends = new WorkCollection<ArraySegment<byte>, SendAsyncResult, Outcome>(ByteArrayComparer.Instance);
            this.lastFlowRequestTime = DateTime.UtcNow;
        }

        public void RegisterCreditListener(Action<uint, bool, ArraySegment<byte>> creditListener)
        {
            if (Interlocked.Exchange(ref this.creditListener, creditListener) != null)
            {
                throw new InvalidOperationException(CommonResources.CreditListenerAlreadyRegistered);
            }
        }

        public void RegisterDispositionListener(Action<Delivery> dispositionListener)
        {
            if (Interlocked.Exchange(ref this.dispositionListener, dispositionListener) != null)
            {
                throw new InvalidOperationException(CommonResources.DispositionListenerAlreadyRegistered);
            }
        }

        public void SendMessageNoWait(
            AmqpMessage message, 
            ArraySegment<byte> deliveryTag, 
            ArraySegment<byte> txnId)
        {
            this.SendMessageInternal(message, deliveryTag, txnId);
        }

        public Task<Outcome> SendMessageAsync(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                (p, t, k, c, s) => ((SendingAmqpLink)s).BeginSendMessage(p.Message, p.DeliveryTag, p.TxnId, t, k, c, s),
                r => ((SendingAmqpLink)r.AsyncState).EndSendMessage(r),
                new SendMessageParam(message, deliveryTag, txnId),
                timeout,
                CancellationToken.None,
                this);
        }

        public Task<Outcome> SendMessageAsync(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (p, t, k, c, s) => ((SendingAmqpLink)s).BeginSendMessage(p.Message, p.DeliveryTag, p.TxnId, t, k, c, s),
                r => ((SendingAmqpLink)r.AsyncState).EndSendMessage(r),
                new SendMessageParam(message, deliveryTag, txnId),
                this.OperationTimeout,
                cancellationToken,
                this);
        }

        public IAsyncResult BeginSendMessage(
            AmqpMessage message,
            ArraySegment<byte> deliveryTag,
            ArraySegment<byte> txnId,
            TimeSpan timeout,
            AsyncCallback callback,
            object state)
        {
            return this.BeginSendMessage(message, deliveryTag, txnId, timeout, CancellationToken.None, callback, state);
        }

        internal IAsyncResult BeginSendMessage(
            AmqpMessage message,
            ArraySegment<byte> deliveryTag,
            ArraySegment<byte> txnId,
            TimeSpan timeout,
            CancellationToken cancellationToken,
            AsyncCallback callback,
            object state)
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

            if (message.SerializedMessageSize == 0)
            {
                throw new InvalidOperationException(AmqpResources.AmqpEmptyMessageNotAllowed);
            }

            return new SendAsyncResult(this, message, deliveryTag, txnId, timeout, cancellationToken, callback, state);
        }

        public Outcome EndSendMessage(IAsyncResult result)
        {
            return SendAsyncResult.End(result);
        }

        public override bool CreateDelivery(Transfer transfer, out Delivery delivery)
        {
            throw new NotImplementedException();
        }

        public override uint Available
        {
            get
            {
                return (uint)this.pendingDeliveries.Count;
            }
        }

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

        protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame frame)
        {
            throw new AmqpException(AmqpErrorCode.NotAllowed, null);
        }

        protected override void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId)
        {
            if (this.LinkCredit > 0)
            {
                this.pendingDeliveries.ContinueWork();
            }

            if (this.LinkCredit > 0 && this.creditListener != null)
            {
                this.creditListener(link, drain, txnId);
            }
        }

        protected override bool CloseInternal()
        {
            this.AbortDeliveries();
            return base.CloseInternal();
        }

        protected override void AbortInternal()
        {
            this.AbortDeliveries();
            base.AbortInternal();
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

        struct SendMessageParam
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
                if (this.CancelInflight())
                {
                    base.CompleteOnTimer();
                }
            }

            bool CancelInflight()
            {
                if (this.link.inflightSends.RemoveWork(this.deliveryTag, this))
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
