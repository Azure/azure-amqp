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

        public ReceivingAmqpLink(AmqpLinkSettings settings)
            : this(null, settings)
        {
        }

        public ReceivingAmqpLink(AmqpSession session, AmqpLinkSettings settings) :
            base("receiver", session, settings)
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
                    long totalSize = 0;
                    if (this.TotalCacheSizeInBytes.HasValue)
                    {
                        totalSize = this.TotalCacheSizeInBytes.Value;
                    }

                    return totalSize - queue.CacheSizeCredit;
                }

                return 0;
            }
        }

        public void SetCacheSizeInBytes(long? cacheSizeInBytes)
        {
            if (cacheSizeInBytes != this.Settings.TotalCacheSizeInBytes)
            {
                this.Settings.TotalCacheSizeInBytes = cacheSizeInBytes;
                SizeBasedFlowQueue queue = this.messageQueue;
                if (queue != null)
                {
                    queue.SetLinkCreditUsingTotalCacheSize();
                }
            }
        }

        public void RegisterMessageListener(Action<AmqpMessage> messageListener)
        {
            if (Interlocked.Exchange(ref this.messageListener, messageListener) != null)
            {
                throw new InvalidOperationException(CommonResources.MessageListenerAlreadyRegistered);
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
                return true;
            }
        }

        protected override bool OpenInternal()
        {
            this.messageQueue = new SizeBasedFlowQueue(this);
            this.waiterList = new LinkedList<ReceiveAsyncResult>();
            this.pendingDispositions = new WorkCollection<ArraySegment<byte>, DisposeAsyncResult, DeliveryState>(ByteArrayComparer.Instance);
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

                        if (this.messageQueue.IsPrefetchingBySize)
                        {
                            if (this.messageQueue.UpdateCreditToIssue(message))
                            {
                                this.SetTotalLinkCredit(this.messageQueue.BoundedTotalLinkCredit, true);
                            }
                        }

                        firstWaiter.Add(message);
                        if (firstWaiter.RequestedMessageCount == 1 || firstWaiter.MessageCount >= firstWaiter.RequestedMessageCount)
                        {
                            this.waiterList.RemoveFirst();
                            firstWaiter.OnRemoved();
                            creditToIssue = this.Settings.AutoSendFlow ? 0 : this.GetOnDemandReceiveCredit();
                            waiter = firstWaiter;
                        }
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

                if (!link.DisposeDelivery(deliveryTag, false, deliveryState, batchable))
                {
                    // Delivery tag not found
                    link.pendingDispositions.CompleteWork(deliveryTag, true, AmqpConstants.RejectedNotFoundOutcome);
                }

                this.StartTracking();
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
                if (this.link.pendingDispositions.TryRemoveWork(this.deliveryTag, out _))
                {
                    this.CompleteSelf(isSynchronous, new TaskCanceledException());
                }
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
                if (this.link.pendingDispositions.TryRemoveWork(this.deliveryTag, out var disposeAsyncResult))
                {
                    base.CompleteOnTimer();
                }
            }
        }

        /// <summary>
        /// The different this class from the normal Queue is that
        /// if we specify a size then we will perform Amqp Flow based on 
        /// the size, where:
        /// - When en-queuing (cache message from service) we will keep sending credit flow as long as size is not over the cache limit.
        /// - When de-queuing (return message to user) we will issue flow as soon as the size is below the threshold (70%).
        /// - When issuing credit we always issue in increment of boundedTotalLinkCredit
        /// - boundedTotalLinkCredit is based on [total cache size] / [max message size]
        /// - [total cache size] has a 90% cap to try to prevent overflow - but does not enforce it.
        /// - we keep updating [max message size] as we receive message in flow.
        /// </summary>
        sealed class SizeBasedFlowQueue : Queue<AmqpMessage>
        {
            // Basically we stop issuing credit at 90% of queue size, and start again at 50%
            const double IssueCreditThresholdRatio = 0.5;
            const double StoppCreditThresholdRatio = 0.9;
            const long DefaultMessageSizeForCacheSizeCalulation = 256 * 1024;
            const uint maxCreditToIssuePerFlow = 500;
            readonly ReceivingAmqpLink receivingLink;

            // the amount of credit that we have, in terms of size. This is used to calculate boundedTotalLinkCredit
            // Note: think of this as the "free space" that we can still fit message with.
            long cacheSizeCredit;

            // average message size that gets updated as message flows in. This is used to calculate boundedTotalLinkCredit
            long inQueueAverageMessageSizeInBytes;

            // the amount of credit in size at which we will start issuing credit again if amount of data is less than this threshold
            long thresholdCacheSizeInBytes;

            // the amount of credit in size at which we will stop issuing credit to avoid an overflow
            long overflowBufferCacheSizeInBytes;

            // the actual credit that we issue over the flow. This can be bounded by maxCreditToIssuePerFlow
            uint boundedTotalLinkCredit;

            public SizeBasedFlowQueue(ReceivingAmqpLink receivingLink)
            {
                Fx.AssertAndThrow(receivingLink != null, "Receive link should not be null");
                Fx.AssertAndThrow(receivingLink.Settings != null, "Setting should not be null");
                this.receivingLink = receivingLink;
                this.SetLinkCreditUsingTotalCacheSize(true);
            }

            internal long AverageMessageSizeInBytes
            {
                get
                {
                    return this.inQueueAverageMessageSizeInBytes > 0 ? this.inQueueAverageMessageSizeInBytes : DefaultMessageSizeForCacheSizeCalulation;
                }
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

            internal uint BoundedTotalLinkCredit
            {
                get
                {
                    return this.boundedTotalLinkCredit;
                }
            }

            public void SetLinkCreditUsingTotalCacheSize(bool setTotalLinkCredit = false)
            {
                if (this.receivingLink.Settings != null && this.IsPrefetchingBySize)
                {
                    this.cacheSizeCredit = this.receivingLink.Settings.TotalCacheSizeInBytes ?? 0;
                    this.thresholdCacheSizeInBytes = (long)(this.cacheSizeCredit * IssueCreditThresholdRatio);
                    this.overflowBufferCacheSizeInBytes = (long)(this.cacheSizeCredit * (1 - StoppCreditThresholdRatio));
                    this.boundedTotalLinkCredit = Convert.ToUInt32(this.cacheSizeCredit / this.AverageMessageSizeInBytes);
                    if (this.boundedTotalLinkCredit <= 0)
                    {
                        // safe guard against cacheSizeCredit being set too low.
                        this.boundedTotalLinkCredit = 1;
                    }

                    if (this.boundedTotalLinkCredit > maxCreditToIssuePerFlow)
                    {
                        this.boundedTotalLinkCredit = maxCreditToIssuePerFlow;
                    }

                    this.receivingLink.LinkCredit = this.boundedTotalLinkCredit;
                    if (setTotalLinkCredit)
                    {
                        this.receivingLink.Settings.TotalLinkCredit = this.boundedTotalLinkCredit;
                    }

                    // This will turn on AutoFlow
                    this.receivingLink.SetTotalLinkCredit(this.boundedTotalLinkCredit, true, true);
                }
            }

            public new void Enqueue(AmqpMessage amqpMessage)
            {
                if (amqpMessage != null)
                {
                    base.Enqueue(amqpMessage);
                    if (this.IsPrefetchingBySize)
                    {
                        this.cacheSizeCredit -= amqpMessage.SerializedMessageSize;
                        bool issueCredit = false;
                        if (this.cacheSizeCredit > 0 && this.cacheSizeCredit > this.overflowBufferCacheSizeInBytes)
                        {
                            issueCredit = this.UpdateCreditToIssue();
                        }
                        else if (this.cacheSizeCredit <= 0)
                        {
                            issueCredit = this.boundedTotalLinkCredit != 0;
                            this.boundedTotalLinkCredit = 0;
                        }
                        else
                        {
                            issueCredit = this.boundedTotalLinkCredit != 1;
                            this.boundedTotalLinkCredit = 1;
                        }

                        if (issueCredit)
                        {
                            this.receivingLink.SetTotalLinkCredit(this.boundedTotalLinkCredit, true);
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

                    bool issueCredit = false;
                    if (this.cacheSizeCredit >= this.thresholdCacheSizeInBytes)
                    {
                        issueCredit = this.UpdateCreditToIssue();
                    }
                    else if (this.cacheSizeCredit > 0)
                    {
                        issueCredit = this.boundedTotalLinkCredit != 1;
                        this.boundedTotalLinkCredit = 1;
                    }

                    if (issueCredit)
                    {
                        this.receivingLink.SetTotalLinkCredit(this.boundedTotalLinkCredit, true);
                    }
                }

                return amqpMessage;
            }

            /// <summary>
            /// This method updates the credit that we will send to service to fetch more
            /// data based on the current average message size. Only call this method if
            /// we already check and cache credit is within range (&gt; 0%, &lt; 90%).
            /// </summary>
            internal bool UpdateCreditToIssue(AmqpMessage externalMessage = null)
            {
                var previousCredit = this.boundedTotalLinkCredit;
                long totalSize = this.receivingLink.Settings.TotalCacheSizeInBytes ?? 0;
                long externalMessageSize = externalMessage == null ? 0 : externalMessage.SerializedMessageSize;
                int count = externalMessage == null ? this.Count : this.Count + 1;
                if (count > 0)
                {
                    this.inQueueAverageMessageSizeInBytes = (totalSize - (this.cacheSizeCredit - externalMessageSize)) / count;
                    if (this.inQueueAverageMessageSizeInBytes <= 0)
                    {
                        this.inQueueAverageMessageSizeInBytes = DefaultMessageSizeForCacheSizeCalulation;
                    }
                }

                // if cacheSizeCredit is negative that means we are already overflowing.
                this.boundedTotalLinkCredit = this.cacheSizeCredit > 0 ? Convert.ToUInt32(this.cacheSizeCredit / this.AverageMessageSizeInBytes) : 0;
                if (this.boundedTotalLinkCredit <= 0 && this.cacheSizeCredit > 0)
                {
                    // this condition can only be possible if average message size is larger than the cache credit.
                    // This is not possible in public stamp as we enforce cache size to be larger than 260k and max message size
                    // is 256k. However this assumption can be broken - e.g. in private stamp max message size can be 1Mb.
                    // since technically cache is not full, we set it to 1.
                    this.boundedTotalLinkCredit = 1;
                }

                if (this.boundedTotalLinkCredit > maxCreditToIssuePerFlow)
                {
                    this.boundedTotalLinkCredit = maxCreditToIssuePerFlow;
                }

                return previousCredit != this.boundedTotalLinkCredit;
            }
        }
    }
}
