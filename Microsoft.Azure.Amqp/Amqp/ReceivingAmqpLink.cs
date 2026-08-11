// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
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
        const int PendingReceiversThreshold = 20;    // after this we will batch credit to minimize flows

        Action<AmqpMessage> messageListener;
        SizeBasedFlowQueue messageQueue;
        WorkCollection<ArraySegment<byte>, DisposeAsyncResult, DeliveryState> pendingDispositions;
        AmqpMessage currentMessage;
        LinkedList<ReceiveAsyncResult> waiterList;
        HashSet<DrainAsyncResult> drainTasks;

        public ReceivingAmqpLink(AmqpLinkSettings settings)
            : this(null, settings)
        {
        }

        public ReceivingAmqpLink(AmqpSession session, AmqpLinkSettings settings) :
            base("receiver", session, settings)
        {
        }

        /// <summary>
        /// Gets the target size, in bytes, of the local receive cache.
        /// </summary>
        /// <remarks>
        /// The target is a soft prefetch hint. The cache can temporarily exceed
        /// it while accepting messages authorized by the current AMQP credit
        /// window.
        /// </remarks>
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
                SizeBasedFlowQueue queue = this.messageQueue;
                return queue != null ? queue.Count : 0;
            }
        }

        internal long AvgMessageSize
        {
            get
            {
                SizeBasedFlowQueue queue = this.messageQueue;
                return queue != null ? queue.AverageMessageSizeInBytes : 0;
            }
        }

        internal long MessageQueueSize
        {
            get
            {
                SizeBasedFlowQueue queue = this.messageQueue;
                if (queue != null && queue.IsPrefetchingBySize)
                {
                    return queue.QueuedBytes;
                }

                return 0;
            }
        }

        /// <summary>
        /// Sets the target size, in bytes, of the local receive cache.
        /// </summary>
        /// <param name="cacheSizeInBytes">
        /// The soft cache-size target, or <see langword="null"/> to disable
        /// size-based prefetch.
        /// </param>
        /// <remarks>
        /// Changing the target does not revoke credit already advertised to the
        /// remote peer. For a finite active credit window, the new target is
        /// applied when that window completes. An open link with unlimited
        /// credit remains unchanged until it is recreated because it has no safe
        /// window boundary. Authorized messages can therefore temporarily cause
        /// the cache to exceed the target. Size-based prefetch does not apply
        /// when a message listener is registered because listener delivery does
        /// not use the local receive cache.
        /// </remarks>
        public void SetCacheSizeInBytes(long? cacheSizeInBytes)
        {
            lock (this.SyncRoot)
            {
                if (cacheSizeInBytes != this.Settings.TotalCacheSizeInBytes)
                {
                    this.Settings.TotalCacheSizeInBytes = cacheSizeInBytes;
                    SizeBasedFlowQueue queue = this.messageQueue;
                    if (queue != null && this.messageListener == null)
                    {
                        queue.SetCacheSize(cacheSizeInBytes);
                    }
                }
            }
        }

        public void RegisterMessageListener(Action<AmqpMessage> messageListener)
        {
            if (Interlocked.Exchange(ref this.messageListener, messageListener) != null)
            {
                throw new InvalidOperationException(CommonResources.MessageListenerAlreadyRegistered);
            }

            lock (this.SyncRoot)
            {
                if (this.messageQueue != null && this.messageQueue.IsPrefetchingBySize)
                {
                    this.messageQueue.DisableSizeBasedPrefetch();
                }
            }
        }

        public IAsyncResult BeginReceiveRemoteMessages(int messageCount, TimeSpan batchWaitTimeout, TimeSpan timeout, AsyncCallback callback, object state)
        {
            // If the caller expects some messages and pass TimeSpan.Zero, we wait to mimic a service call
            if (timeout == TimeSpan.Zero && !this.Settings.AutoSendFlow)
            {
                timeout = MinReceiveTimeout;
            }

            return this.BeginReceiveMessages(messageCount, batchWaitTimeout, timeout, CancellationToken.None, callback, state);
        }

        public Task<AmqpMessage> ReceiveMessageAsync(TimeSpan timeout)
        {
            return this.ReceiveMessageAsync(timeout, CancellationToken.None);
        }

        public Task<AmqpMessage> ReceiveMessageAsync(CancellationToken cancellationToken)
        {
            return this.ReceiveMessageAsync(this.OperationTimeout, cancellationToken);
        }

        public Task<AmqpMessage> ReceiveMessageAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (t, k, c, s) => ((ReceivingAmqpLink)s).BeginReceiveMessages(1, TimeSpan.Zero, t, k, c, s),
                r =>
                {
                    ((ReceivingAmqpLink)r.AsyncState).EndReceiveMessages(r, out var messages);
                    return messages.FirstOrDefault();
                },
                timeout,
                cancellationToken,
                this);
        }

        public Task<IEnumerable<AmqpMessage>> ReceiveMessagesAsync(int messageCount, TimeSpan batchWaitTimeout)
        {
            return this.ReceiveMessagesAsync(messageCount, batchWaitTimeout, this.OperationTimeout, CancellationToken.None);
        }

        public Task<IEnumerable<AmqpMessage>> ReceiveMessagesAsync(int messageCount, TimeSpan batchWaitTimeout, CancellationToken cancellationToken)
        {
            return this.ReceiveMessagesAsync(messageCount, batchWaitTimeout, this.OperationTimeout, cancellationToken);
        }

        public Task<IEnumerable<AmqpMessage>> ReceiveMessagesAsync(int messageCount, TimeSpan batchWaitTimeout, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (p, c, s) => ((ReceivingAmqpLink)s).BeginReceiveMessages(p.MessageCount, p.BatchWaitTime, p.Timeout, p.CancellationToken, c, s),
                r => { ((ReceivingAmqpLink)r.AsyncState).EndReceiveMessages(r, out var messages); return messages; },
                new ReceiveParam(messageCount, batchWaitTimeout, timeout, cancellationToken),
                this);
        }

        public IAsyncResult BeginReceiveMessage(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginReceiveMessages(1, TimeSpan.Zero, timeout, CancellationToken.None, callback, state);
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
            return BeginReceiveMessages(messageCount, TimeSpan.Zero, timeout, CancellationToken.None, callback, state);
        }

        IAsyncResult BeginReceiveMessages(int messageCount, TimeSpan batchWaitTimeout, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            this.ThrowIfClosed();
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

            if (messages.Count == 0 && timeout > TimeSpan.Zero)
            {
                ReceiveAsyncResult waiter = new ReceiveAsyncResult(this, messageCount, batchWaitTimeout, timeout, callback, state);
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
                        waiter.Initialize(node, cancellationToken);
                        completeWaiter = false;

                        // If no auto-flow, trigger a flow to get messages.
                        int creditToIssue = this.Settings.AutoSendFlow || this.messageQueue.IsPrefetchingBySize ?
                            0 :
                            this.GetOnDemandReceiveCredit();
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

        public bool EndReceiveMessages(IAsyncResult result, out IEnumerable<AmqpMessage> messages)
        {
            if (result is ReceiveAsyncResult)
            {
                return ReceiveAsyncResult.End(result, out messages);
            }

            messages = CompletedAsyncResult<IEnumerable<AmqpMessage>>.End(result);
            return true;
        }

        public Task DrainAsyc(CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (thisPtr, k, c, s) => new DrainAsyncResult(thisPtr, thisPtr.OperationTimeout, k, c, s),
                r => DrainAsyncResult.End(r),
                this,
                cancellationToken,
                this);
        }

        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, TimeSpan timeout)
        {
            return this.DisposeMessageAsync(deliveryTag, AmqpConstants.NullBinary, outcome, batchable, timeout);
        }

        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable, TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                (p, t, k, c, s) => ((ReceivingAmqpLink)s).BeginDisposeMessage(p.DeliveryTag, p.TxnId, p.Outcome, p.Batchable, t, k, c, s),
                r => ((ReceivingAmqpLink)r.AsyncState).EndDisposeMessage(r),
                new DisposeParam(deliveryTag, txnId, outcome, batchable),
                timeout,
                CancellationToken.None,
                this);
        }

        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, CancellationToken cancellationToken)
        {
            return this.DisposeMessageAsync(deliveryTag, AmqpConstants.NullBinary, outcome, batchable, cancellationToken);
        }

        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (p, t, k, c, s) => ((ReceivingAmqpLink)s).BeginDisposeMessage(p.DeliveryTag, p.TxnId, p.Outcome, p.Batchable, t, k, c, s),
                r => ((ReceivingAmqpLink)r.AsyncState).EndDisposeMessage(r),
                new DisposeParam(deliveryTag, txnId, outcome, batchable),
                this.OperationTimeout,
                cancellationToken,
                this);
        }

        public IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginDisposeMessage(deliveryTag, AmqpConstants.NullBinary, outcome, batchable, timeout, CancellationToken.None, callback, state);
        }

        public IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginDisposeMessage(deliveryTag, txnId, outcome, batchable, timeout, CancellationToken.None, callback, state);
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
            this.ThrowIfClosed();
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
                SizeBasedFlowQueue queue = this.messageQueue;
                if (queue != null && queue.IsPrefetchingBySize)
                {
                    lock (this.SyncRoot)
                    {
                        if (queue == this.messageQueue && queue.IsPrefetchingBySize)
                        {
                            queue.OnDeliveryStarted();
                        }
                    }
                }

                return true;
            }
        }

        protected override bool OpenInternal()
        {
            this.messageQueue = new SizeBasedFlowQueue(this);
            this.waiterList = new LinkedList<ReceiveAsyncResult>();
            this.pendingDispositions = new WorkCollection<ArraySegment<byte>, DisposeAsyncResult, DeliveryState>(ByteArrayComparer.Instance);
            if (this.messageListener == null && this.Settings.TotalCacheSizeInBytes.HasValue)
            {
                this.messageQueue.EnableSizeBasedPrefetch(true);
            }

            bool syncComplete = base.OpenInternal();
            lock (this.SyncRoot)
            {
                if (this.messageQueue.IsPrefetchingBySize)
                {
                    this.messageQueue.TryIssueNextWindow();
                }
                else if (this.LinkCredit > 0)
                {
                    this.SendFlow(false);
                }
            }

            return syncComplete;
        }

        protected override void OnDisposeDeliveryInternal(Delivery delivery)
        {
            // This happens when the sender sends a disposition after the receiver's disposition
            // in the EO delivery scenario, and also in transaction case.
            AmqpTrace.Provider.AmqpDispose(this, delivery.DeliveryId.Value, delivery.Settled, delivery.State);
            DeliveryState deliveryState = delivery.State;

            if (deliveryState != null)
            {
                this.pendingDispositions.CompleteWork(delivery.DeliveryTag, false, deliveryState);
            }
        }

        protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame frame)
        {
            Fx.Assert(delivery == null || delivery == this.currentMessage, "The delivery must be null or must be the same as the current message.");
            if (this.Settings.MaxMessageSize.HasValue && this.Settings.MaxMessageSize.Value > 0)
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
            frame.RawByteBuffer.AddReference();    // Message also owns the buffer from now on
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
                    this.DisposeDelivery(message, true, AmqpConstants.ReleasedOutcome);
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

        protected override void OnReceiveFlow(Flow flow)
        {
            bool draining = this.Drain;
            base.OnReceiveFlow(flow);
            if (draining && this.LinkCredit == 0)
            {
                HashSet<DrainAsyncResult> pendingTasks = null;
                lock (this.SyncRoot)
                {
                    pendingTasks = this.drainTasks;
                    this.drainTasks = null;
                }

                if (pendingTasks != null)
                {
                    foreach (var task in pendingTasks)
                    {
                        task.Signal(false);
                    }
                }
            }
        }

        IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable,
            TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            this.ThrowIfClosed();
            return new DisposeAsyncResult(this, deliveryTag, txnId, outcome, batchable, timeout, cancellationToken, callback, state);
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
                            waiter.Signal(false, null);
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
                SizeBasedFlowQueue queue = this.messageQueue;
                if (queue != null && queue.IsPrefetchingBySize)
                {
                    lock (this.SyncRoot)
                    {
                        if (queue == this.messageQueue && queue.IsPrefetchingBySize)
                        {
                            queue.TrackReceivedMessage(message);
                        }
                    }
                }

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
                        var firstWaiter = this.waiterList.First.Value;

                        this.messageQueue.TrackReceivedMessage(message);

                        firstWaiter.Add(message);
                        if (firstWaiter.RequestedMessageCount == 1 || firstWaiter.MessageCount >= firstWaiter.RequestedMessageCount)
                        {
                            this.waiterList.RemoveFirst();
                            firstWaiter.OnRemoved();
                            creditToIssue = this.Settings.AutoSendFlow || this.messageQueue.IsPrefetchingBySize ?
                                0 :
                                this.GetOnDemandReceiveCredit();
                            waiter = firstWaiter;
                        }
                    }
                    else if (this.messageQueue != null &&
                        !this.Settings.AutoSendFlow &&
                        !this.messageQueue.IsPrefetchingBySize &&
                        this.Settings.SettleType != SettleMode.SettleOnSend)
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
                    ActionItem.Schedule(o => { var w = (ReceiveAsyncResult)o; w.Signal(false); }, waiter);
                }
            }
        }

        // Must be called with lock held
        int GetOnDemandReceiveCredit()
        {
            Fx.Assert(!this.Settings.AutoSendFlow, "This is only valid when auto-flow is false");
            int credit = 0;
            int currentCredit = (int)this.LinkCredit;
            int totalRequestedMessageCount = 0;
            foreach (var waiter in this.waiterList)
            {
                totalRequestedMessageCount += waiter.RequestedMessageCount;
            }

            if (this.waiterList.Count == totalRequestedMessageCount)
            {
                if (this.waiterList.Count > currentCredit &&
                    currentCredit < MaxCreditForOnDemandReceive)
                {
                    int needCredit = Math.Min(this.waiterList.Count, MaxCreditForOnDemandReceive) - currentCredit;
                    if (this.waiterList.Count <= CreditBatchThreshold ||
                        currentCredit == 0 ||
                        needCredit % CreditBatchThreshold == 0)
                    {
                        credit = currentCredit + needCredit;
                    }
                }
            }
            else
            {
                if (totalRequestedMessageCount > currentCredit)
                {
                    int needCredit = totalRequestedMessageCount - currentCredit;
                    if (this.waiterList.Count <= PendingReceiversThreshold ||
                        currentCredit == 0 ||
                        this.waiterList.Count % PendingReceiversThreshold == 0)
                    {
                        credit = currentCredit + needCredit;
                    }
                }
            }

            return credit;
        }

        struct ReceiveParam
        {
            public ReceiveParam(int messageCount, TimeSpan batchWaitTime, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.MessageCount = messageCount;
                this.BatchWaitTime = batchWaitTime;
                this.Timeout = timeout;
                this.CancellationToken = cancellationToken;
            }

            public readonly int MessageCount;
            public readonly TimeSpan BatchWaitTime;
            public readonly TimeSpan Timeout;
            public readonly CancellationToken CancellationToken;
        }

        struct DisposeParam
        {
            public DisposeParam(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable)
            {
                this.DeliveryTag = deliveryTag;
                this.TxnId = txnId;
                this.Outcome = outcome;
                this.Batchable = batchable;
            }

            public readonly ArraySegment<byte> DeliveryTag;
            public readonly ArraySegment<byte> TxnId;
            public readonly Outcome Outcome;
            public readonly bool Batchable;
        }

        sealed class ReceiveAsyncResult : AsyncResult
        {
            readonly ReceivingAmqpLink parent;
            readonly int requestedMessageCount;
            readonly TimeSpan batchWaitTimeout;
            readonly TimeSpan timeout;
            CancellationTokenRegistration cancellationTokenRegistration;
            Timer timer;
            LinkedListNode<ReceiveAsyncResult> node;
            int completed;  // 1: signaled, 2: timeout
            List<AmqpMessage> messages;

            public ReceiveAsyncResult(ReceivingAmqpLink parent, int requestedMessageCount, TimeSpan batchWaitTimeout,
                TimeSpan timeout, AsyncCallback callback, object state)
                : base(callback, state)
            {
                this.parent = parent;
                this.batchWaitTimeout = batchWaitTimeout;
                this.requestedMessageCount = requestedMessageCount;
                Fx.Assert(timeout > TimeSpan.Zero, "must have a non-zero timeout");
                this.timeout = timeout;
            }

            public int RequestedMessageCount
            {
                get
                {
                    return this.requestedMessageCount;
                }
            }

            public int MessageCount
            {
                get
                {
                    return this.messages != null ? this.messages.Count : 0;
                }
            }

            public void Initialize(LinkedListNode<ReceiveAsyncResult> node, CancellationToken cancellationToken)
            {
                this.node = node;
                if (this.timeout != TimeSpan.MaxValue)
                {
                    this.timer = new Timer(s => OnTimer(s), this, this.timeout, Timeout.InfiniteTimeSpan);
                }
                if (cancellationToken.CanBeCanceled)
                {
                    this.cancellationTokenRegistration = cancellationToken.Register(o =>
                    {
                        ReceiveAsyncResult result = (ReceiveAsyncResult)o;
                        RemoveFromWaiterList(result);
                        result.Signal(false, new TaskCanceledException());
                    }, this);
                }
            }

            // Needs caller to hold lock to ReceivingAmqpLink.SyncRoot
            public void Add(AmqpMessage message)
            {
                if (this.messages == null)
                {
                    this.messages = new List<AmqpMessage>();
                    this.messages.Add(message);
                    if (this.requestedMessageCount > 1 && this.batchWaitTimeout != TimeSpan.MaxValue)
                    {
                        this.timer.Change(this.batchWaitTimeout, Timeout.InfiniteTimeSpan);
                    }
                }
                else
                {
                    this.messages.Add(message);
                }
            }

            public static bool End(IAsyncResult result, out IEnumerable<AmqpMessage> messages)
            {
                ReceiveAsyncResult thisPtr = AsyncResult.End<ReceiveAsyncResult>(result);
                messages = thisPtr.messages != null ? thisPtr.messages : AmqpConstants.EmptyMessages;
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
                    this.Signal(false, new OperationCanceledException(this.parent.TerminalException.Message, this.parent.TerminalException));
                }
                else
                {
                    this.Signal(false, new OperationCanceledException());
                }
            }

            public void Signal(bool syncComplete)
            {
                this.Signal(syncComplete, null);
            }

            public void Signal(List<AmqpMessage> messages, bool syncComplete)
            {
                if (this.messages != null)
                {
                    this.messages.AddRange(messages);
                }
                else
                {
                    this.messages = messages;
                }

                this.Signal(syncComplete, null);
            }

            public void Signal(bool syncComplete, Exception exception)
            {
                this.CompleteInternal(syncComplete, 1, exception);
            }

            void CompleteInternal(bool syncComplete, int code, Exception exception)
            {
                Timer t = this.timer;
                if (t != null)
                {
                    t.Dispose();
                }

                this.cancellationTokenRegistration.Dispose();
                if (Interlocked.CompareExchange(ref this.completed, code, 0) == 0)
                {
                    if (this.messages == null)
                    {
                        this.messages = new List<AmqpMessage>();
                    }

                    if (exception != null)
                    {
                        this.Complete(syncComplete, exception);
                    }
                    else
                    {
                        this.Complete(syncComplete);
                    }
                }
            }

            internal static void OnTimer(object state)
            {
                ReceiveAsyncResult thisPtr = (ReceiveAsyncResult)state;
                RemoveFromWaiterList(thisPtr);

                thisPtr.CompleteInternal(false, thisPtr.MessageCount > 0 ? 1 : 2, null); // 1: signaled, 2: timeout
            }

            private static void RemoveFromWaiterList(ReceiveAsyncResult result)
            {
                lock (result.parent.SyncRoot)
                {
                    if (result.parent.waiterList == null || result.node == null)
                    {
                        return;
                    }

                    result.parent.waiterList.Remove(result.node);
                    result.node = null;
                }
            }
        }

        sealed class DrainAsyncResult : TimeoutAsyncResult<string>
        {
            readonly ReceivingAmqpLink link;

            public DrainAsyncResult(ReceivingAmqpLink link,
                TimeSpan timeout,
                CancellationToken cancellationToken,
                AsyncCallback callback,
                object state)
                : base(timeout, cancellationToken, callback, state)
            {
                this.link = link;
                this.Start();
            }

            public static void End(IAsyncResult result)
            {
                AsyncResult.End<DrainAsyncResult>(result);
            }

            public void Signal(bool isSynchronous)
            {
                this.CompleteSelf(isSynchronous);
            }

            public override void Cancel(bool isSynchronous)
            {
                if (this.Remove())
                {
                    this.CompleteSelf(isSynchronous, new TaskCanceledException());
                }
            }

            protected override string Target
            {
                get { return "drain"; }
            }

            protected override void CompleteOnTimer()
            {
                if (this.Remove())
                {
                    base.CompleteOnTimer();
                }
            }

            void Start()
            {
                lock (this.link.SyncRoot)
                {
                    if (this.link.drainTasks == null)
                    {
                        this.link.drainTasks = new HashSet<DrainAsyncResult>();
                    }

                    this.link.drainTasks.Add(this);
                    if (!this.link.Drain)
                    {
                        this.link.SendFlow(false, true, null);
                    }
                }

                this.StartTracking();
            }

            bool Remove()
            {
                lock (this.link.SyncRoot)
                {
                    if (this.link.drainTasks == null || !this.link.drainTasks.Remove(this))
                    {
                        return false;
                    }

                    if (this.link.drainTasks.Count == 0)
                    {
                        this.link.drainTasks = null;
                    }
                }

                return true;
            }
        }

        sealed class DisposeAsyncResult : TimeoutAsyncResult<string>, IWork<DeliveryState>
        {
            readonly ReceivingAmqpLink link;
            readonly ArraySegment<byte> deliveryTag;
            readonly bool batchable;
            Outcome outcome;
            ArraySegment<byte> txnId;

            public DisposeAsyncResult(
                ReceivingAmqpLink link,
                ArraySegment<byte> deliveryTag,
                ArraySegment<byte> txnId,
                Outcome outcome,
                bool batchable,
                TimeSpan timeout,
                CancellationToken cancellationToken,
                AsyncCallback callback,
                object state)
                : base(timeout, cancellationToken, callback, state)
            {
                this.link = link;
                this.deliveryTag = deliveryTag;
                this.batchable = batchable;
                this.outcome = outcome;
                this.txnId = txnId;
                this.link.pendingDispositions.StartWork(deliveryTag, this);
            }

            public static Outcome End(IAsyncResult result)
            {
                return AsyncResult.End<DisposeAsyncResult>(result).outcome;
            }

            public void Start()
            {
                DeliveryState deliveryState;
                if (txnId.Array != null)
                {
                    deliveryState = new TransactionalState()
                    {
                        Outcome = this.outcome,
                        TxnId = this.txnId
                    };
                }
                else
                {
                    deliveryState = this.outcome;
                }

                if (!this.link.DisposeDelivery(deliveryTag, false, deliveryState, batchable))
                {
                    // Delivery tag not found
                    this.link.pendingDispositions.RemoveWork(this.deliveryTag, this);
                    this.Done(true, AmqpConstants.RejectedNotFoundOutcome);
                }
                else
                {
                    this.StartTracking();
                }
            }

            public void Done(bool completedSynchronously, DeliveryState state)
            {
                if (state is Outcome outcome)
                {
                    this.outcome = outcome;
                }
                else
                {
                    if (state is TransactionalState transactionalState)
                    {
                        this.outcome = transactionalState.Outcome;
                    }
                    else
                    {
                        this.CompleteSelf(completedSynchronously, new AmqpException(AmqpErrorCode.IllegalState, $"DeliveryState '{state.GetType()}' is not valid for disposition."));
                        return;
                    }
                }

                this.CompleteSelf(completedSynchronously);
            }

            public override void Cancel(bool isSynchronous)
            {
                this.link.pendingDispositions.RemoveWork(this.deliveryTag, this);
                this.CompleteSelf(isSynchronous, new TaskCanceledException());
            }

            public void Cancel(bool completedSynchronously, Exception exception)
            {
                this.CompleteSelf(completedSynchronously, exception);
            }

            protected override string Target
            {
                get { return "dispose"; }
            }

            protected override void CompleteOnTimer()
            {
                // Timeout
                this.link.pendingDispositions.RemoveWork(this.deliveryTag, this);
                base.CompleteOnTimer();
            }
        }

        /// <summary>
        /// Tracks queued message bytes and issues discrete size-based credit
        /// windows without revoking credit already advertised to the peer.
        /// </summary>
        /// <remarks>
        /// Credit is estimated by dividing currently available cache bytes by
        /// the average serialized size observed in the previous completed
        /// window. The first window uses a 256-KB estimate, every window is
        /// limited to 500 messages, and a non-positive target uses one-message
        /// windows.
        ///
        /// An active window is never topped up or reduced. All messages
        /// authorized by that window are accepted, so the byte target is a soft
        /// hint and the queue can temporarily overshoot it. A new window is
        /// issued only after the current window is exhausted, its final delivery
        /// is complete, and the queue has capacity.
        ///
        /// State transitions:
        ///
        ///   No active window
        ///          |
        ///          v
        ///   Calculate credit from free bytes / estimated message size
        ///          |
        ///          v
        ///   Issue one bounded window
        ///          |
        ///          v
        ///   Accept all authorized deliveries and collect size samples
        ///          |
        ///          v
        ///   Window exhausted and final fragmented delivery complete
        ///          |
        ///          +-- queue at or above target --&gt; pause until dequeue
        ///          |
        ///          +-- queue below target --------&gt; issue next window
        /// </remarks>
        sealed class SizeBasedFlowQueue : Queue<AmqpMessage>
        {
            const long DefaultMessageSizeForCacheSizeCalulation = 256 * 1024;
            const uint maxCreditToIssuePerFlow = 500;
            readonly ReceivingAmqpLink receivingLink;
            long targetCacheSizeInBytes;
            long queuedBytes;
            long estimatedMessageSizeInBytes;
            long windowMessageBytes;
            int windowMessageCount;
            uint issuedWindowSize;
            uint countBasedTotalLinkCredit;
            bool countBasedAutoSendFlow;
            volatile bool isPrefetchingBySize;
            bool disableSizeBasedPrefetch;
            bool deliveryInProgress;

            public SizeBasedFlowQueue(ReceivingAmqpLink receivingLink)
            {
                Fx.AssertAndThrow(receivingLink != null, "Receive link should not be null");
                Fx.AssertAndThrow(receivingLink.Settings != null, "Setting should not be null");
                this.receivingLink = receivingLink;
                this.estimatedMessageSizeInBytes = DefaultMessageSizeForCacheSizeCalulation;
            }

            internal long AverageMessageSizeInBytes
            {
                get
                {
                    return this.estimatedMessageSizeInBytes;
                }
            }

            internal bool IsPrefetchingBySize
            {
                get
                {
                    return this.isPrefetchingBySize;
                }
            }

            internal long CacheSizeCredit
            {
                get
                {
                    return this.targetCacheSizeInBytes - this.queuedBytes;
                }
            }

            internal long QueuedBytes
            {
                get
                {
                    return this.queuedBytes;
                }
            }

            internal uint BoundedTotalLinkCredit
            {
                get
                {
                    return this.issuedWindowSize;
                }
            }

            public void EnableSizeBasedPrefetch(bool initializeLinkCredit)
            {
                Fx.Assert(this.receivingLink.Settings.TotalCacheSizeInBytes.HasValue, "Cache size must be set.");
                this.countBasedTotalLinkCredit = this.receivingLink.Settings.TotalLinkCredit;
                this.countBasedAutoSendFlow = this.receivingLink.Settings.AutoSendFlow;
                this.targetCacheSizeInBytes = this.receivingLink.Settings.TotalCacheSizeInBytes.Value;
                this.windowMessageBytes = 0;
                this.windowMessageCount = 0;
                this.issuedWindowSize = this.receivingLink.LinkCredit;
                this.disableSizeBasedPrefetch = false;
                this.isPrefetchingBySize = true;

                if (initializeLinkCredit)
                {
                    this.receivingLink.InitializeLinkCredit(0, false);
                    this.issuedWindowSize = 0;
                }
                else
                {
                    this.receivingLink.Settings.AutoSendFlow = false;
                }
            }

            public void SetCacheSize(long? cacheSizeInBytes)
            {
                if (cacheSizeInBytes.HasValue)
                {
                    if (this.IsPrefetchingBySize)
                    {
                        this.targetCacheSizeInBytes = cacheSizeInBytes.Value;
                        this.disableSizeBasedPrefetch = false;
                        this.TryIssueNextWindow();
                    }
                    else
                    {
                        // Unlimited credit has no safe window boundary at which it can be revoked.
                        // Keep the current link unchanged; the setting will apply when a new link opens.
                        if (this.receivingLink.LinkCredit < uint.MaxValue)
                        {
                            this.EnableSizeBasedPrefetch(false);
                            this.TryIssueNextWindow();
                        }
                    }
                }
                else if (this.IsPrefetchingBySize)
                {
                    this.DisableSizeBasedPrefetch();
                }
            }

            public void DisableSizeBasedPrefetch()
            {
                this.disableSizeBasedPrefetch = true;
                this.TryIssueNextWindow();
            }

            public new void Enqueue(AmqpMessage amqpMessage)
            {
                if (amqpMessage == null)
                {
                    return;
                }

                base.Enqueue(amqpMessage);
                this.queuedBytes += amqpMessage.SerializedMessageSize;
                this.TrackReceivedMessage(amqpMessage);
            }

            public new AmqpMessage Dequeue()
            {
                AmqpMessage amqpMessage = base.Dequeue();
                if (amqpMessage != null)
                {
                    this.queuedBytes -= amqpMessage.SerializedMessageSize;
                    Fx.Assert(this.queuedBytes >= 0, "Queued message size cannot be negative.");
                    this.TryIssueNextWindow();
                }

                return amqpMessage;
            }

            public void TrackReceivedMessage(AmqpMessage amqpMessage)
            {
                if (!this.IsPrefetchingBySize || amqpMessage == null)
                {
                    return;
                }

                this.windowMessageBytes += amqpMessage.SerializedMessageSize;
                ++this.windowMessageCount;
                this.deliveryInProgress = false;
                if (!this.receivingLink.HasOutstandingCredit)
                {
                    this.CompleteWindow();
                    this.TryIssueNextWindow();
                }
            }

            public void OnDeliveryStarted()
            {
                if (this.IsPrefetchingBySize)
                {
                    this.deliveryInProgress = true;
                }
            }

            public void TryIssueNextWindow()
            {
                if (!this.IsPrefetchingBySize ||
                    this.deliveryInProgress ||
                    this.receivingLink.HasOutstandingCredit)
                {
                    return;
                }

                if (this.disableSizeBasedPrefetch)
                {
                    if (this.receivingLink.TryIssueCredit(
                        this.countBasedTotalLinkCredit,
                        this.countBasedAutoSendFlow,
                        false,
                        AmqpConstants.NullBinary))
                    {
                        this.issuedWindowSize = 0;
                        this.disableSizeBasedPrefetch = false;
                        this.isPrefetchingBySize = false;
                    }

                    return;
                }

                if (!this.HasFreeCapacity())
                {
                    return;
                }

                uint credit = this.CalculateNextWindow();
                if (this.receivingLink.TryIssueCredit(credit, false, false, AmqpConstants.NullBinary))
                {
                    this.issuedWindowSize = credit;
                }
            }

            void CompleteWindow()
            {
                if (this.windowMessageCount > 0)
                {
                    long average = this.windowMessageBytes / this.windowMessageCount;
                    this.estimatedMessageSizeInBytes = average > 0 ? average : DefaultMessageSizeForCacheSizeCalulation;
                }

                this.windowMessageBytes = 0;
                this.windowMessageCount = 0;
                this.issuedWindowSize = 0;
            }

            bool HasFreeCapacity()
            {
                return this.targetCacheSizeInBytes <= 0 ?
                    this.queuedBytes == 0 :
                    this.queuedBytes < this.targetCacheSizeInBytes;
            }

            uint CalculateNextWindow()
            {
                if (this.targetCacheSizeInBytes <= 0)
                {
                    return 1;
                }

                // Use the previous completed window as the estimate for the next
                // window. This is deliberately a hint: actual message sizes can
                // vary, but already-issued credit must not be revoked.
                long freeBytes = this.targetCacheSizeInBytes - this.queuedBytes;
                long estimatedSize = this.AverageMessageSizeInBytes;
                long calculatedCredit = freeBytes / estimatedSize;
                if (calculatedCredit <= 0)
                {
                    calculatedCredit = 1;
                }
                else if (calculatedCredit > maxCreditToIssuePerFlow)
                {
                    calculatedCredit = maxCreditToIssuePerFlow;
                }

                return (uint)calculatedCredit;
            }
        }
    }
}