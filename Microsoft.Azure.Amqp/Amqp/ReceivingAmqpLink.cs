// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
        Justification = "Uses custom scheme for cleanup")]
    public sealed class ReceivingAmqpLink : AmqpLink
    {
        // Workaround for TimeSpan.Zero server wait timeout. Consider supporting this with drain
        static readonly TimeSpan MinReceiveTimeout = TimeSpan.FromSeconds(10);

        // To support non-prefetch mode with multiple BeginReceive calls
        const int MaxCreditForOnDemandReceive = 200;
        const int CreditBatchThreshold = 20;    // after this we will batch credit to minimize flows

        Action<AmqpMessage> messageListener;
        SizeBasedFlowQueue messageQueue;
        WorkCollection<ArraySegment<byte>, DisposeAsyncResult, Outcome> pendingDispositions;
        AmqpMessage currentMessage;
        LinkedList<ReceiveAsyncResult> waiterList;

        public ReceivingAmqpLink(AmqpLinkSettings settings)
            : this(null, settings)
        {
        }

        public ReceivingAmqpLink(AmqpSession session, AmqpLinkSettings settings) :
            base(session, settings)
        {
        }

        public long? TotalCacheSizeInBytes
        {
            get
            {
                return this.Settings.TotalCacheSizeInBytes;
            }
        }

        internal int MessageQueueCount
        {
            get
            {
                return this.messageQueue != null ? this.messageQueue.Count : 0;
            }
        }

        internal long MessageQueueSize
        {
            get
            {
                if (this.messageQueue != null &&
                    this.messageQueue.IsPrefetchingBySize)
                {
                    return this.messageQueue.CacheSizeCredit;
                }

                return 0;
            }
        }

        public void SetCacheSizeInBytes(long? cacheSizeInBytes)
        {
            if (cacheSizeInBytes != this.Settings.TotalCacheSizeInBytes)
            {
                this.Settings.TotalCacheSizeInBytes = cacheSizeInBytes;
                this.messageQueue.SetLinkCreditUsingTotalCacheSize();
            }
        }


        public void RegisterMessageListener(Action<AmqpMessage> messageListener)
        {
            if (Interlocked.Exchange(ref this.messageListener, messageListener) != null)
            {
                throw new InvalidOperationException(CommonResources.MessageListenerAlreadyRegistered);
            }
        }

        public IAsyncResult BeginReceiveRemoteMessage(TimeSpan timeout, AsyncCallback callback, object state)
        {
            // If the caller expects some messages and pass TimeSpan.Zero, we wait to mimic a service call
            if (timeout == TimeSpan.Zero)
            {
                timeout = MinReceiveTimeout;
            }

            return this.BeginReceiveMessage(timeout, callback, state);
        }

        public Task<AmqpMessage> ReceiveMessageAsync(TimeSpan timeout)
        {
            return TaskHelpers.CreateTask(
                (c, s) => ((ReceivingAmqpLink)s).BeginReceiveMessage(timeout, c, s),
                (a) =>
                {
                    AmqpMessage message;
                    ((ReceivingAmqpLink)a.AsyncState).EndReceiveMessage(a, out message);
                    return message;
                },
                this);
        }

        public IAsyncResult BeginReceiveMessage(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginReceiveMessages(1, timeout, callback, state);
        }

        public bool EndReceiveMessage(IAsyncResult result, out AmqpMessage message)
        {
            if (result is ReceiveAsyncResult)
            {
                IEnumerable<AmqpMessage> messages;
                bool retValue = ReceiveAsyncResult.End(result, out messages);
                message = messages.FirstOrDefault();
                return retValue;
            }

            message = CompletedAsyncResult<IEnumerable<AmqpMessage>>.End(result).FirstOrDefault();
            return true;
        }

        public IAsyncResult BeginReceiveMessages(int messageCount, TimeSpan timeout, AsyncCallback callback, object state)
        {
            List<AmqpMessage> messages = new List<AmqpMessage>();
            lock (this.SyncRoot)
            {
                if (this.messageQueue != null && this.messageQueue.Count > 0)
                {
                    for (int i = 0; i < messageCount && this.messageQueue.Count > 0; i++)
                    {
                        messages.Add(this.messageQueue.Dequeue());
                    }
                }
            }

            if (!messages.Any() && timeout > TimeSpan.Zero)
            {
                ReceiveAsyncResult waiter = new ReceiveAsyncResult(this, timeout, callback, state);
                bool completeWaiter = true;
                lock (this.SyncRoot)
                {
                    if (this.messageQueue == null)
                    {
                        // closed, so return null message immediately
                    }
                    else if (this.messageQueue.Count > 0)
                    {
                        for (int i = 0; i < messageCount && this.messageQueue.Count > 0; i++)
                        {
                            messages.Add(this.messageQueue.Dequeue());
                        }
                    }
                    else
                    {
                        LinkedListNode<ReceiveAsyncResult> node = this.waiterList.AddLast(waiter);
                        waiter.Initialize(node);
                        completeWaiter = false;

                        // If no auto-flow, trigger a flow to get messages.
                        int creditToIssue = this.Settings.AutoSendFlow ? 0 : this.GetOnDemandReceiveCredit();
                        if (creditToIssue > 0)
                        {
                            // Before the credit is issued, waiters could be completed already. In this case, we will queue the incoming
                            // messages and wait for the next receive calls.
                            this.IssueCredit((uint)creditToIssue, false, AmqpConstants.NullBinary);
                        }
                    }
                }

                if (completeWaiter)
                {
                    waiter.Signal(messages, true);
                }

                return waiter;
            }

            return new CompletedAsyncResult<IEnumerable<AmqpMessage>>(messages, callback, state);
        }

        protected override void OnReceiveStateOpenSent(Attach attach)
        {
            // If we uses prefetchBySize logic, then we need to
            // Update link credit based on Gateway's return max message size
            lock (this.SyncRoot)
            {
                var queue = this.messageQueue as SizeBasedFlowQueue;
                if (queue != null)
                {
                    queue.SetLinkCreditUsingTotalCacheSize();
                }
            }
        }

        public bool EndReceiveMessages(IAsyncResult result, out IEnumerable<AmqpMessage> messages)
        {
            if (result is ReceiveAsyncResult)
            {
                return ReceiveAsyncResult.End(result, out messages);
            }

            messages = CompletedAsyncResult<IEnumerable<AmqpMessage>>.End(result);
            return true;
        }

        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, TimeSpan timeout)
        {
            return TaskHelpers.CreateTask(
                (c, s) => this.BeginDisposeMessage(deliveryTag, outcome, batchable, timeout, c, s),
                a => ((ReceivingAmqpLink)a.AsyncState).EndDisposeMessage(a),
                this);
        }

        public IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return new DisposeAsyncResult(this, deliveryTag, outcome, batchable, timeout, callback, state);
        }

        public Outcome EndDisposeMessage(IAsyncResult result)
        {
            return DisposeAsyncResult.End(result);
        }

        public void AcceptMessage(AmqpMessage message, bool batchable)
        {
            bool settled = this.Settings.SettleType != SettleMode.SettleOnDispose;
            this.AcceptMessage(message, settled, batchable);
        }

        public void AcceptMessage(AmqpMessage message, bool settled, bool batchable)
        {
            this.DisposeMessage(message, AmqpConstants.AcceptedOutcome, settled, batchable);
        }

        public void RejectMessage(AmqpMessage message, Exception exception)
        {
            Rejected rejected = new Rejected();
            rejected.Error = Error.FromException(exception);

            this.DisposeMessage(message, rejected, true, false);
        }

        public void ReleaseMessage(AmqpMessage message)
        {
            this.DisposeMessage(message, AmqpConstants.ReleasedOutcome, true, false);
        }

        public void ModifyMessage(AmqpMessage message, bool deliveryFailed, bool deliverElseWhere, Fields messageAttributes)
        {
            Modified modified = new Modified();
            modified.DeliveryFailed = deliveryFailed;
            modified.UndeliverableHere = deliverElseWhere;
            modified.MessageAnnotations = messageAttributes;

            this.DisposeMessage(message, modified, true, false);
        }

        public void DisposeMessage(AmqpMessage message, DeliveryState state, bool settled, bool batchable)
        {
            message.Batchable = batchable;
            this.DisposeDelivery(message, settled, state);
        }

        public override bool CreateDelivery(Transfer transfer, out Delivery delivery)
        {
            if (this.currentMessage != null)
            {
                delivery = this.currentMessage;
                return false;
            }
            else
            {
                delivery = this.currentMessage = AmqpMessage.CreateReceivedMessage();
                return true;
            }
        }

        protected override bool OpenInternal()
        {
            this.messageQueue = new SizeBasedFlowQueue(this);
            this.waiterList = new LinkedList<ReceiveAsyncResult>();
            this.pendingDispositions = new WorkCollection<ArraySegment<byte>, DisposeAsyncResult, Outcome>(ByteArrayComparer.Instance);
            bool syncComplete = base.OpenInternal();
            if (this.LinkCredit > 0)
            {
                this.SendFlow(false);
            }

            return syncComplete;
        }

        protected override void OnDisposeDeliveryInternal(Delivery delivery)
        {
            // This happens when the sender sends a disposition after the receiver's disposition
            // in the EO delivery scenario, and also in transaction case.
            AmqpTrace.Provider.AmqpDispose(this, delivery.DeliveryId.Value, delivery.Settled, delivery.State);
            DeliveryState deliveryState = delivery.State;
            if (delivery.Transactional())
            {
                deliveryState = ((TransactionalState)delivery.State).Outcome;
            }

            if (deliveryState != null)
            {
                this.pendingDispositions.CompleteWork(delivery.DeliveryTag, false, (Outcome)deliveryState);
            }
        }

        protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame frame)
        {
            Fx.Assert(delivery == null || delivery == this.currentMessage, "The delivery must be null or must be the same as the current message.");
            if (this.Settings.MaxMessageSize.HasValue)
            {
                ulong size = (ulong)(this.currentMessage.BytesTransfered + frame.Payload.Count);
                if (size > this.Settings.MaxMessageSize.Value)
                {
                    if (this.IsClosing())
                    {
                        // The closing sequence has been started, so any 
                        // transfer is meaningless, so we can treat them as no-op
                        return;
                    }

                    throw new AmqpException(AmqpErrorCode.MessageSizeExceeded,
                        AmqpResources.GetString(AmqpResources.AmqpMessageSizeExceeded, this.currentMessage.DeliveryId.Value, size, this.Settings.MaxMessageSize.Value));
                }
            }

            Fx.Assert(this.currentMessage != null, "Current message must have been created!");
            ArraySegment<byte> payload = frame.Payload;
            frame.RawByteBuffer.AdjustPosition(payload.Offset, payload.Count);
            frame.RawByteBuffer.Clone();    // Message also owns the buffer from now on
            this.currentMessage.AddPayload(frame.RawByteBuffer, !transfer.More());
            if (!transfer.More())
            {
                AmqpMessage message = this.currentMessage;
                this.currentMessage = null;

                AmqpTrace.Provider.AmqpReceiveMessage(this, message.DeliveryId.Value, message.RawByteBuffers.Count);
                this.OnReceiveMessage(message);
            }
        }

        protected override void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId)
        {
        }

        protected override void AbortInternal()
        {
            Queue<AmqpMessage> messages = null;
            this.CancelPendingOperations(true, out messages);
            
            if (messages != null)
            {
                foreach (AmqpMessage message in messages)
                {
                    message.Dispose();
                }
            }

            AmqpMessage temp = this.currentMessage;
            if (temp != null)
            {
                temp.Dispose();
            }

            base.AbortInternal();
        }

        protected override bool CloseInternal()
        {
            Queue<AmqpMessage> messages = null;
            this.CancelPendingOperations(false, out messages);

            if (messages != null)
            {
                foreach (AmqpMessage message in messages)
                {
                    this.ReleaseMessage(message);
                    message.Dispose();
                }
            }

            AmqpMessage temp = this.currentMessage;
            if (temp != null)
            {
                temp.Dispose();
            }

            return base.CloseInternal();
        }

        void CancelPendingOperations(bool aborted, out Queue<AmqpMessage> messagesToRelease)
        {
            messagesToRelease = null;
            LinkedList<ReceiveAsyncResult> waiters = null;
            lock (this.SyncRoot)
            {
                messagesToRelease = this.messageQueue;
                waiters = this.waiterList;
                this.messageQueue = null;
                this.waiterList = null;
            }

            if (waiters != null)
            {
                ActionItem.Schedule(o =>
                    {
                        var state = (Tuple<LinkedList<ReceiveAsyncResult>, bool>)o;
                        LinkedList<ReceiveAsyncResult> waitersToCancel = state.Item1;
                        foreach (ReceiveAsyncResult waiter in waitersToCancel)
                        {
                            if (state.Item2)
                            {
                                waiter.Cancel();
                            }
                            else
                            {
                                waiter.Signal(null, false, null);
                            }
                        }
                    },
                    new Tuple<LinkedList<ReceiveAsyncResult>, bool>(waiters, aborted));
            }

            if (this.pendingDispositions != null)
            {
                this.pendingDispositions.Abort();
            }
        }

        void OnReceiveMessage(AmqpMessage message)
        {
            if (this.messageListener != null)
            {
                this.messageListener(message);
            }
            else
            {
                ReceiveAsyncResult waiter = null;
                int creditToIssue = 0;
                bool releaseMessage = false;
                lock (this.SyncRoot)
                {
                    if (this.waiterList != null && this.waiterList.Count > 0)
                    {
                        waiter = this.waiterList.First.Value;
                        this.waiterList.RemoveFirst();
                        waiter.OnRemoved();

                        creditToIssue = this.Settings.AutoSendFlow ? 0 : this.GetOnDemandReceiveCredit();
                    }
                    else if (!this.Settings.AutoSendFlow && this.Settings.SettleType != SettleMode.SettleOnSend)
                    {
                        releaseMessage = true;
                    }
                    else if (this.messageQueue != null)
                    {
                        this.messageQueue.Enqueue(message);
                        AmqpTrace.Provider.AmqpCacheMessage(
                            this,
                            message.DeliveryId.Value,
                            this.messageQueue.Count,
                            this.messageQueue.IsPrefetchingBySize,
                            this.TotalCacheSizeInBytes ?? 0,
                            this.Settings == null ? 0 : this.Settings.TotalLinkCredit,
                            this.LinkCredit);
                    }
                }

                if (releaseMessage)
                {
                    this.ReleaseMessage(message);
                    message.Dispose();
                }

                if (creditToIssue > 0)
                {
                    this.IssueCredit((uint)creditToIssue, false, AmqpConstants.NullBinary);
                }

                if (waiter != null)
                {
                    // Schedule the completion on another thread so we don't block the I/O thread
                    ActionItem.Schedule(
                        o => { var state = (Tuple<ReceiveAsyncResult, IEnumerable<AmqpMessage>>)o; state.Item1.Signal(state.Item2, false); },
                        new Tuple<ReceiveAsyncResult, IEnumerable<AmqpMessage>>(waiter, new AmqpMessage[] { message }));
                }
            }
        }

        // Must be called with lock held
        int GetOnDemandReceiveCredit()
        {
            Fx.Assert(!this.Settings.AutoSendFlow, "This is only valid when auto-flow is false");
            int credit = 0;
            int currentCredit = (int)this.LinkCredit;
            if (this.waiterList.Count > currentCredit &&
                currentCredit < MaxCreditForOnDemandReceive)
            {
                int needCredit = Math.Min(this.waiterList.Count, MaxCreditForOnDemandReceive) - currentCredit;
                if (this.waiterList.Count <= CreditBatchThreshold || needCredit % CreditBatchThreshold == 0)
                {
                    credit = currentCredit + needCredit;
                }
            }

            return credit;
        }

        sealed class ReceiveAsyncResult : AsyncResult
        {
            readonly ReceivingAmqpLink parent;
            readonly TimeSpan timeout;
            Timer timer;
            LinkedListNode<ReceiveAsyncResult> node;
            int completed;  // 1: signaled, 2: timeout
            IEnumerable<AmqpMessage> messages;

            public ReceiveAsyncResult(ReceivingAmqpLink parent, TimeSpan timeout, AsyncCallback callback, object state)
                : base(callback, state)
            {
                this.parent = parent;
                Fx.Assert(timeout > TimeSpan.Zero, "must have a non-zero timeout");
                this.timeout = timeout;
            }

            public void Initialize(LinkedListNode<ReceiveAsyncResult> node)
            {
                this.node = node;
                if (this.timeout != TimeSpan.MaxValue)
                {
                    timer = new Timer(s => OnTimer(s), this, this.timeout, Timeout.InfiniteTimeSpan);
                }
            }

            public static bool End(IAsyncResult result, out IEnumerable<AmqpMessage> messages)
            {
                ReceiveAsyncResult thisPtr = AsyncResult.End<ReceiveAsyncResult>(result);
                messages = thisPtr.messages;
                return thisPtr.completed == 1;
            }

            // Ensure the lock is held when calling this function
            public void OnRemoved()
            {
                this.node = null;
            }

            public void Cancel()
            {
                if (this.parent.TerminalException != null)
                {
                    this.Signal(null, false, new OperationCanceledException(this.parent.TerminalException.Message, this.parent.TerminalException));
                }
                else
                {
                    this.Signal(null, false, new OperationCanceledException());
                }
            }

            public void Signal(IEnumerable<AmqpMessage> messages, bool syncComplete)
            {
                this.Signal(messages, syncComplete, null);
            }

            public void Signal(IEnumerable<AmqpMessage> messages, bool syncComplete, Exception exception)
            {
                Timer t = this.timer;
                if (t != null)
                {
                    t.Change(Timeout.Infinite, Timeout.Infinite);
                }

                this.CompleteInternal(messages, syncComplete, 1, exception);
            }

            void CompleteInternal(IEnumerable<AmqpMessage> messages, bool syncComplete, int code, Exception exception)
            {
                if (Interlocked.CompareExchange(ref this.completed, code, 0) == 0)
                {
                    this.messages = messages;
                    if (messages == null)
                    {
                        this.messages = Enumerable.Empty<AmqpMessage>();
                    }

                    if(exception != null)
                    {
                        this.Complete(syncComplete, exception);
                    }
                    else
                    {
                        this.Complete(syncComplete);
                    }
                }
            }

            static void OnTimer(object state)
            {
                ReceiveAsyncResult thisPtr = (ReceiveAsyncResult)state;
                lock (thisPtr.parent.SyncRoot)
                {
                    if (thisPtr.parent.waiterList == null || thisPtr.node == null)
                    {
                        return;
                    }

                    thisPtr.parent.waiterList.Remove(thisPtr.node);
                    thisPtr.node = null;
                }

                thisPtr.CompleteInternal(null, false, 2, null);
            }
        }

        sealed class DisposeAsyncResult : AsyncResult, IWork<Outcome>
        {
            readonly ReceivingAmqpLink link;
            readonly ArraySegment<byte> deliveryTag;
            readonly bool batchable;
            Outcome outcome;

            public DisposeAsyncResult(
                ReceivingAmqpLink link,
                ArraySegment<byte> deliveryTag, 
                Outcome outcome, 
                bool batchable, 
                TimeSpan timeout, 
                AsyncCallback callback, 
                object state)
                : base(callback, state)
            {
                this.link = link;
                this.deliveryTag = deliveryTag;
                this.batchable = batchable;
                this.outcome = outcome;
                this.link.pendingDispositions.StartWork(deliveryTag, this);
            }

            public static Outcome End(IAsyncResult result)
            {
                return AsyncResult.End<DisposeAsyncResult>(result).outcome;
            }

            public void Start()
            {
                if (!link.DisposeDelivery(deliveryTag, false, outcome, batchable))
                {
                    // Delivery tag not found
                    link.pendingDispositions.CompleteWork(deliveryTag, true, AmqpConstants.RejectedNotFoundOutcome);
                }
            }

            public void Done(bool completedSynchronously, Outcome outcome)
            {
                this.outcome = outcome;
                this.Complete(completedSynchronously);
            }

            public void Cancel(bool completedSynchronously, Exception exception)
            {
                this.Complete(completedSynchronously, exception);
            }
        }

        /// <summary>
        /// The different this class from the normal Queue is that
        /// if we specify a size then we will perform Amqp Flow based on 
        /// the size, where:
        /// - When en-queuing we will keep sending credit flow as long as size is not over the limit.
        /// - When de-queuing we will issue flow as soon as the size is below the threshold.
        /// - When issuing credit we always issue in increment of boundedTotalLinkCredit
        /// - boundedTotalLinkCredit is based on [total cache size] / [max message size]
        /// </summary>
        /// <typeparam name="T"></typeparam>
        sealed class SizeBasedFlowQueue : Queue<AmqpMessage>
        {
            const long DefaultMessageSizeForCacheSizeCalulation = 256 * 1024;
            readonly ReceivingAmqpLink receivingLink;
            long cacheSizeCredit;
            long maxMessageSizeInBytes;
            long thresholdCacheSizeInBytes;
            uint boundedTotalLinkCredit;

            public SizeBasedFlowQueue(ReceivingAmqpLink receivingLink)
            {
                Fx.AssertAndThrow(receivingLink != null, "Receive link should not be null");
                Fx.AssertAndThrow(receivingLink.Settings != null, "Setting should not be null");
                this.receivingLink = receivingLink;
                this.SetLinkCreditUsingTotalCacheSize(true);
            }

            internal bool IsPrefetchingBySize
            {
                get
                {
                    return this.receivingLink.Settings.TotalCacheSizeInBytes.HasValue;
                }
            }

            internal long CacheSizeCredit
            {
                get
                {
                    return this.cacheSizeCredit;
                }
            }
            
            public void SetLinkCreditUsingTotalCacheSize(bool setTotalLinkCredit = false)
            {
                if (this.receivingLink.Settings != null && this.IsPrefetchingBySize)
                {
                    this.maxMessageSizeInBytes = DefaultMessageSizeForCacheSizeCalulation;
                    this.cacheSizeCredit = this.receivingLink.Settings.TotalCacheSizeInBytes ?? 0;
                    this.thresholdCacheSizeInBytes = this.receivingLink.Settings.TotalCacheSizeInBytes.Value / 2;
                    if (this.receivingLink.Settings.MaxMessageSize.HasValue &&
                        this.receivingLink.Settings.MaxMessageSize <= Convert.ToUInt64(long.MaxValue))
                    {
                        this.maxMessageSizeInBytes = Convert.ToInt64(this.receivingLink.Settings.MaxMessageSize, CultureInfo.InvariantCulture);
                    }

                    this.boundedTotalLinkCredit = Convert.ToUInt32(this.receivingLink.Settings.TotalCacheSizeInBytes / this.maxMessageSizeInBytes, CultureInfo.InvariantCulture);

                    this.receivingLink.LinkCredit = this.boundedTotalLinkCredit;
                    if (setTotalLinkCredit)
                    {
                        this.receivingLink.Settings.TotalLinkCredit = this.boundedTotalLinkCredit;
                    }

                    this.receivingLink.SetTotalLinkCredit(this.boundedTotalLinkCredit, true);
                }
            }

            public new void Enqueue(AmqpMessage amqpMessage)
            {
                if (amqpMessage != null)
                {
                    base.Enqueue(amqpMessage);
                    if (this.IsPrefetchingBySize)
                    {
                        // Note that currently we don't actively stop prefetch even if cache size credit is reached because
                        // the original total credit was using the worst case scenario as calculation anyway so we will just
                        // let credit run dry on its own. cacheSizeCredit can run into negative.
                        this.cacheSizeCredit -= amqpMessage.SerializedMessageSize;
                        if (this.cacheSizeCredit > 0)
                        {
                            this.receivingLink.IssueCredit(this.boundedTotalLinkCredit, false, AmqpConstants.NullBinary);
                        }
                    }
                }
            }

            public new AmqpMessage Dequeue()
            {
                var amqpMessage = base.Dequeue();
                if (amqpMessage != null && this.IsPrefetchingBySize)
                {
                    // if we are draining the cache (i.e. receiving) we should start giving
                    // credit if we now have credit above the threshold.
                    this.cacheSizeCredit += amqpMessage.SerializedMessageSize;
                    if (this.cacheSizeCredit >= this.thresholdCacheSizeInBytes)
                    {
                        this.receivingLink.IssueCredit(this.boundedTotalLinkCredit, false, AmqpConstants.NullBinary);
                    }
                }

                return amqpMessage;
            }
        }
    }
}
