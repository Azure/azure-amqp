﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// A link for receiving messages.
    /// </summary>
    public sealed class ReceivingAmqpLink : AmqpLink
    {
        // Workaround for TimeSpan.Zero server wait timeout. Consider supporting this with drain
        static readonly TimeSpan MinReceiveTimeout = TimeSpan.FromSeconds(10);
        readonly ConcurrentQueue<AmqpMessage> messageQueue;
        readonly WorkCollection<ArraySegment<byte>, DisposeAsyncResult, DeliveryState> pendingDispositions;
        readonly WaiterManager waiterManager;
        readonly PrefetchSizeTracker prefetchSizeTracker;
        Action<AmqpMessage> messageListener;
        AmqpMessage currentMessage;
        int checkWaiterCount;

        /// <summary>
        /// Initializes the receiver link without attaching to any session.
        /// </summary>
        /// <param name="settings">The link settings.</param>
        public ReceivingAmqpLink(AmqpLinkSettings settings)
            : this(null, settings)
        {
        }

        /// <summary>
        /// Initializes the receiver link and attach it to the session.
        /// </summary>
        /// <param name="session">The session to attach the link.</param>
        /// <param name="settings">The link settings.</param>
        public ReceivingAmqpLink(AmqpSession session, AmqpLinkSettings settings) :
            base("receiver", session, settings)
        {
            this.messageQueue = new ConcurrentQueue<AmqpMessage>();
            this.pendingDispositions = new WorkCollection<ArraySegment<byte>, DisposeAsyncResult, DeliveryState>(ByteArrayComparer.Instance);
            this.waiterManager = new WaiterManager(this);
            if (settings.TotalCacheSizeInBytes != null)
            {
                this.prefetchSizeTracker = new PrefetchSizeTracker(this, settings.TotalCacheSizeInBytes.Value);
                this.prefetchSizeTracker.Update();
            }
        }

        /// <summary>
        /// Registers a message listener to handle received messages.
        /// </summary>
        /// <param name="messageListener">The message listener.</param>
        public void RegisterMessageListener(Action<AmqpMessage> messageListener)
        {
            if (Interlocked.Exchange(ref this.messageListener, messageListener) != null)
            {
                throw new InvalidOperationException(CommonResources.MessageListenerAlreadyRegistered);
            }
        }

        /// <summary>
        /// Starts the operation to receive a message with the default wait timeout.
        /// </summary>
        /// <returns>A message when the task is completed. Null if there is no message available.</returns>
        public Task<AmqpMessage> ReceiveMessageAsync()
        {
            return this.ReceiveMessageAsync(this.OperationTimeout);
        }

        /// <summary>
        /// Starts the operation to receive a message.
        /// </summary>
        /// <param name="timeout">The time to wait for any message.</param>
        /// <returns>A message when the task is completed. Null if there is no message available.</returns>
        public Task<AmqpMessage> ReceiveMessageAsync(TimeSpan timeout)
        {
            return this.ReceiveMessageAsync(timeout, CancellationToken.None);
        }

        /// <summary>
        /// Starts the operation to receive a message. The Operation completes when a message is available or the cancellationToken is cancelled.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A message when the task is completed. Null if there is no message available.</returns>
        public Task<AmqpMessage> ReceiveMessageAsync(CancellationToken cancellationToken)
        {
            return this.ReceiveMessageAsync(TimeSpan.MaxValue, cancellationToken);
        }

        /// <summary>
        /// Starts the operation to receive a message.
        /// </summary>
        /// <param name="timeout">The time to wait for any message.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A message when the task is completed. Null if there is no message available.</returns>
        public Task<AmqpMessage> ReceiveMessageAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (t, k, c, s) => ((ReceivingAmqpLink)s).BeginReceiveMessageBatch(1, TimeSpan.Zero, t, k, c, s),
                static r => { ((ReceivingAmqpLink)r.AsyncState).EndReceiveMessageSingle(r, out var message); return message; },
                timeout,
                cancellationToken,
                this);
        }

        /// <summary>
        /// Starts the operation to receive a batch of messages with default wait timeout.
        /// </summary>
        /// <param name="messageCount">The desired number of messages.</param>
        /// <param name="batchWaitTimeout">The time to wait for more messages in the batch after the first message is available.</param>
        /// <returns>A list of messages when the task is completed. Empty if there is no message available.</returns>
        public Task<IEnumerable<AmqpMessage>> ReceiveMessagesAsync(int messageCount, TimeSpan batchWaitTimeout)
        {
            return this.ReceiveMessagesAsync(messageCount, batchWaitTimeout, this.OperationTimeout, CancellationToken.None);
        }

        /// <summary>
        /// Starts the operation to receive a batch of messages. The Operation completes when message(s) are available or the cancellationToken is cancelled.
        /// </summary>
        /// <param name="messageCount">The desired number of messages.</param>
        /// <param name="batchWaitTimeout">The time to wait for more messages in the batch after the first message is available.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A list of messages when the task is completed. Empty if there is no message available.</returns>
        public Task<IEnumerable<AmqpMessage>> ReceiveMessagesAsync(int messageCount, TimeSpan batchWaitTimeout, CancellationToken cancellationToken)
        {
            return this.ReceiveMessagesAsync(messageCount, batchWaitTimeout, TimeSpan.MaxValue, cancellationToken);
        }

        /// <summary>
        /// Starts the operation to receive a batch of messages.
        /// </summary>
        /// <param name="messageCount">The desired number of messages.</param>
        /// <param name="batchWaitTimeout">The time to wait for more messages in the batch after the first message is available.</param>
        /// <param name="timeout">The time to wait for the first message to become available.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A list of messages when the task is completed. Empty if there is no message available.</returns>
        public Task<IEnumerable<AmqpMessage>> ReceiveMessagesAsync(int messageCount, TimeSpan batchWaitTimeout, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (p, c, s) => ((ReceivingAmqpLink)s).BeginReceiveMessageBatch(p.MessageCount, p.BatchWaitTime, p.Timeout, p.CancellationToken, c, s),
                static r => { ((ReceivingAmqpLink)r.AsyncState).EndReceiveMessageBatch(r, out var messages); return messages; },
                new ReceiveParam(messageCount, batchWaitTimeout, timeout, cancellationToken),
                this);
        }

        /// <summary>
        /// Begins the message receive operation. The operation returns immediately
        /// when no message is available in the prefetch cache.
        /// </summary>
        /// <param name="timeout">The time to wait for messages.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginReceiveMessage(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginReceiveMessageBatch(1, TimeSpan.Zero, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Begins the message receive operation. The operation waits at least 10 seconds
        /// when no messages are available.
        /// </summary>
        /// <param name="messageCount">The requested message count.</param>
        /// <param name="batchWaitTimeout">The wait time for the messages.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginReceiveRemoteMessages(int messageCount, TimeSpan batchWaitTimeout, TimeSpan timeout, AsyncCallback callback, object state)
        {
            // If the caller expects some messages and pass TimeSpan.Zero, we wait to mimic a service call
            if (timeout == TimeSpan.Zero && !this.Settings.AutoSendFlow)
            {
                timeout = MinReceiveTimeout;
            }

            return this.BeginReceiveMessageBatch(messageCount, batchWaitTimeout, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Ends the message receive operation.
        /// </summary>
        /// <param name="result">The result returned by the begin method.</param>
        /// <param name="message">The received message. Null if no message is available.</param>
        /// <returns>True if the operation is completed within the specified time; false otherwise.</returns>
        public bool EndReceiveMessage(IAsyncResult result, out AmqpMessage message)
        {
            bool retValue = ReceiveAsyncResult.End(result, out List<AmqpMessage> messages);
            message = messages.Count == 0 ? null : messages[0];
            return retValue;
        }

        /// <summary>
        /// Begins the operation to receive a batch of messages.
        /// </summary>
        /// <param name="messageCount">The desired number of messages.</param>
        /// <param name="timeout">The time to wait for any messages.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginReceiveMessages(int messageCount, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginReceiveMessageBatch(messageCount, TimeSpan.Zero, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Ends the message receive operation.
        /// </summary>
        /// <param name="result">The result returned by the begin method.</param>
        /// <param name="messages">The returned messages.</param>
        /// <returns>True if the operation is completed within the specified time; false otherwise.</returns>
        public bool EndReceiveMessages(IAsyncResult result, out IEnumerable<AmqpMessage> messages)
        {
            bool completed = ReceiveAsyncResult.End(result, out List<AmqpMessage> list);
            messages = list;
            return completed;
        }

        /// <summary>
        /// Updates the outcome of a received message.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="outcome">The outcome.</param>
        /// <returns>An <see cref="Outcome"/> from remote peer when the task is completed.</returns>
        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, Outcome outcome)
        {
            return this.DisposeMessageAsync(deliveryTag, AmqpConstants.NullBinary, outcome, true, this.OperationTimeout);
        }

        /// <summary>
        /// Updates the outcome of a received message.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="outcome">The outcome.</param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An <see cref="Outcome"/> from remote peer when the task is completed.</returns>
        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, TimeSpan timeout)
        {
            return this.DisposeMessageAsync(deliveryTag, AmqpConstants.NullBinary, outcome, batchable, timeout);
        }

        /// <summary>
        /// Updates the outcome of a received message in a transaction.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="outcome">The outcome.</param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>An <see cref="Outcome"/> from remote peer when the task is completed.</returns>
        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable, TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                static (p, t, k, c, s) => ((ReceivingAmqpLink)s).BeginDisposeMessage(p.DeliveryTag, p.TxnId, p.Outcome, p.Batchable, t, k, c, s),
                static r => ((ReceivingAmqpLink)r.AsyncState).EndDisposeMessage(r),
                new DisposeParam(deliveryTag, txnId, outcome, batchable),
                timeout,
                CancellationToken.None,
                this);
        }

        /// <summary>
        /// Updates the outcome of a received message in a transaction, if specified.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="outcome">The outcome.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An <see cref="Outcome"/> from remote peer when the task is completed.</returns>
        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, Outcome outcome, CancellationToken cancellationToken)
        {
            return this.DisposeMessageAsync(deliveryTag, AmqpConstants.NullBinary, outcome, true, cancellationToken);
        }

        /// <summary>
        /// Updates the outcome of a received message in a transaction, if specified.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="outcome">The outcome.</param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>An <see cref="Outcome"/> from remote peer when the task is completed.</returns>
        public Task<Outcome> DisposeMessageAsync(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (p, t, k, c, s) => ((ReceivingAmqpLink)s).BeginDisposeMessage(p.DeliveryTag, p.TxnId, p.Outcome, p.Batchable, t, k, c, s),
                static r => ((ReceivingAmqpLink)r.AsyncState).EndDisposeMessage(r),
                new DisposeParam(deliveryTag, txnId, outcome, batchable),
                this.OperationTimeout,
                cancellationToken,
                this);
        }

        /// <summary>
        /// Begins to update the outcome of a received message.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="outcome">The outcome.</param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, Outcome outcome, bool batchable, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginDisposeMessage(deliveryTag, AmqpConstants.NullBinary, outcome, batchable, timeout, callback, state);
        }

        /// <summary>
        /// Begins to update the outcome of a received message in a transaction.
        /// </summary>
        /// <param name="deliveryTag">The delivery-tag of the message.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="outcome">The outcome.</param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginDisposeMessage(deliveryTag, txnId, outcome, batchable, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Ends the message state update operation.
        /// </summary>
        /// <param name="result">The result returned by the begin method.</param>
        /// <returns>Outcome of the update operation.</returns>
        public Outcome EndDisposeMessage(IAsyncResult result)
        {
            return DisposeAsyncResult.End(result);
        }

        /// <summary>
        /// Accepts a message. The method does not wait for a response from the peer.
        /// </summary>
        /// <param name="message">The message to accept.</param>
        public void AcceptMessage(AmqpMessage message)
        {
            bool settled = this.Settings.SettleType != SettleMode.SettleOnDispose;
            this.AcceptMessage(message, settled, message.Batchable);
        }

        /// <summary>
        /// Accepts a message.
        /// </summary>
        /// <param name="message">The message to accept.</param>
        /// <param name="settled"><see cref="Delivery.Settled"/></param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        public void AcceptMessage(AmqpMessage message, bool settled, bool batchable)
        {
            this.DisposeMessage(message, AmqpConstants.AcceptedOutcome, settled, batchable);
        }

        /// <summary>
        /// Rejects a message. The method does not wait for a response from the peer.
        /// </summary>
        /// <param name="message">The message to reject.</param>
        /// <param name="exception">The error for the rejection.</param>
        public void RejectMessage(AmqpMessage message, Exception exception)
        {
            Rejected rejected = new Rejected();
            rejected.Error = Error.FromException(exception);

            this.DisposeMessage(message, rejected, true, message.Batchable);
        }

        /// <summary>
        /// Releases a message. The method does not wait for a response from the peer.
        /// </summary>
        /// <param name="message">The message to release.</param>
        public void ReleaseMessage(AmqpMessage message)
        {
            this.DisposeMessage(message, AmqpConstants.ReleasedOutcome, true, message.Batchable);
        }

        /// <summary>
        /// Modifies a message.
        /// </summary>
        /// <param name="message">The message to modify.</param>
        /// <param name="deliveryFailed"><see cref="Modified.DeliveryFailed"/></param>
        /// <param name="deliverElseWhere"><see cref="Modified.UndeliverableHere"/></param>
        /// <param name="messageAttributes"><see cref="Modified.MessageAnnotations"/></param>
        public void ModifyMessage(AmqpMessage message, bool deliveryFailed, bool deliverElseWhere, Fields messageAttributes)
        {
            Modified modified = new Modified();
            modified.DeliveryFailed = deliveryFailed;
            modified.UndeliverableHere = deliverElseWhere;
            modified.MessageAnnotations = messageAttributes;

            this.DisposeMessage(message, modified, true, message.Batchable);
        }

        /// <summary>
        /// Updates the state of a message. The method does not wait for a response from the peer.
        /// </summary>
        /// <param name="message">The message to update.</param>
        /// <param name="state">The new delivery state.</param>
        /// <param name="settled"><see cref="Delivery.Settled"/></param>
        /// <param name="batchable"><see cref="Delivery.Batchable"/></param>
        public void DisposeMessage(AmqpMessage message, DeliveryState state, bool settled, bool batchable)
        {
            this.ThrowIfClosed();
            message.Batchable = batchable;
            this.DisposeDelivery(message, settled, state);
        }

        /// <summary>
        /// Creates a delivery for the received transfer. If the transfer is the first of a message,
        /// a delivery object must be created. If it is continous, the current delivery must be returned.
        /// </summary>
        /// <param name="transfer">The received transfer.</param>
        /// <param name="delivery">The returned delivery.</param>
        /// <returns>True if a delivery is created.</returns>
        protected override bool CreateDelivery(Transfer transfer, out Delivery delivery)
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

        /// <summary>
        /// Opens the link.
        /// </summary>
        /// <returns>True if open is completed.</returns>
        protected override bool OpenInternal()
        {
            bool syncComplete = base.OpenInternal();
            if (this.LinkCredit > 0)
            {
                this.SendFlow(false);
            }

            return syncComplete;
        }

        /// <summary>
        /// Called when the state of a delivey is updated by the remote peer. Override this method to perform
        /// other operations if needed.
        /// </summary>
        /// <param name="delivery"></param>
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

        /// <summary>
        /// Called when a transfer is received from the peer.
        /// </summary>
        /// <param name="delivery">Delivery to which the transfer belongs.</param>
        /// <param name="transfer">The received transfer.</param>
        /// <param name="frame">The transfer frame.</param>
        protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame frame)
        {
            bool shouldProcessMessage = !delivery.Aborted;
            if (this.IsRecoverable)
            {
                // If the delivery is already settled and the send mode is not SettleOnSend, the remote sender could have only derived
                // the settled state after being told by the local receiver that it's either settled or arrived at a terminal outcome.
                // In this case, since the local receiver has already settled it or processed it to a terminal outcome, there is no need to process it again.
                if (delivery.Settled && this.Settings.SettleType != SettleMode.SettleOnSend)
                {
                    shouldProcessMessage = false;
                }

                // TODO: should the message be reprocesse if it has already reached terminal outcome?
                //if (delivery.State.IsTerminal() && this.UnsettledMap.TryGetValue(delivery.DeliveryTag, out Delivery localUnsettledDelivery) && localUnsettledDelivery.State.IsTerminal())
                //{
                //    // If both sides has reached terminal states (like Oasis AMQP doc section 3.4.6, example delivery tag 12, 13),
                //    // then we can simply settle it and let the remote sender know, without actually processing it.
                //    shouldProcessMessage = false;
                //}

                if (!shouldProcessMessage)
                {
                    // If both sides have reached terminal states, but the delivery still hasn't been settled,
                    // the receiver should still send a disposition to acknowledge the sender's delivery state and inform the remote sender that the message is settled.
                    if (!delivery.Settled && !delivery.Aborted)
                    {
                        this.DisposeDelivery(delivery, true, delivery.State, false);
                    }
                    else
                    {
                        this.RemoveUnsettledDelivery(delivery.DeliveryTag);
                    }
                }
            }

            if (shouldProcessMessage)
            {
                Fx.Assert(delivery == null || delivery == this.currentMessage, "The delivery must be null or must be the same as the current message.");
                this.AddOrUpdateUnsettledDelivery(delivery);

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

                    AmqpTrace.Provider.AmqpReceiveMessage(this, message.DeliveryId.Value, message.Segments);
                    this.OnReceiveMessage(message);
                }
            }
        }

        /// <summary>
        /// <see cref="AmqpLink.OnCreditAvailable(int, uint, bool, ArraySegment{byte})"/>
        /// </summary>
        /// <param name="session"></param>
        /// <param name="link"></param>
        /// <param name="drain"></param>
        /// <param name="txnId"></param>
        protected override void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId)
        {
        }

        /// <summary>
        /// Aborts the link.
        /// </summary>
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

        /// <summary>
        /// Closes the link.
        /// </summary>
        /// <returns>True if close is completed.</returns>
        protected override bool CloseInternal()
        {
            Queue<AmqpMessage> messages = null;
            this.CancelPendingOperations(false, out messages);

            if (messages != null)
            {
                foreach (AmqpMessage message in messages)
                {
                    this.DisposeDelivery(message, true, AmqpConstants.ReleasedOutcome, false);
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

        /// <summary>
        /// Process and consolidate the unsettled deliveries sent with the remote Attach frame, by checking against the unsettled deliveries for this link terminus.
        /// </summary>
        /// <param name="remoteAttach">The incoming Attach from remote which contains the remote's unsettled delivery states.</param>
        protected override void ProcessUnsettledDeliveries(Attach remoteAttach)
        {
            if (this.Session.Connection.LinkTerminusManager.TryGetLinkTerminus(this.LinkIdentifier, out AmqpLinkTerminus linkTerminus))
            {
                IDictionary<ArraySegment<byte>, Delivery> resultantUnsettledMap = Task.Run(() => linkTerminus.NegotiateUnsettledDeliveriesAsync(remoteAttach)).Result;
                if (resultantUnsettledMap != null)
                {
                    foreach (var resultantUnsettled in resultantUnsettledMap)
                    {
                        this.AddOrUpdateUnsettledDelivery(resultantUnsettled.Value);
                    }
                }
            }
        }

        IAsyncResult BeginReceiveMessageBatch(int messageCount, TimeSpan batchWaitTimeout, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            this.ThrowIfClosed();
            ReceiveAsyncResult waiter = new ReceiveAsyncResult(this, messageCount, batchWaitTimeout, timeout, callback, state);
            this.waiterManager.AddWaiter(waiter, cancellationToken);
            this.CheckWaiter();
            return waiter;
        }

        bool EndReceiveMessageSingle(IAsyncResult result, out AmqpMessage message)
        {
            bool completed = ReceiveAsyncResult.End(result, out List<AmqpMessage> list);
            message = list.Count > 0 ? list[0] : null;
            return completed;
        }

        bool EndReceiveMessageBatch(IAsyncResult result, out IEnumerable<AmqpMessage> messages)
        {
            bool completed = ReceiveAsyncResult.End(result, out List<AmqpMessage> list);
            messages = list;
            return completed;
        }

        IAsyncResult BeginDisposeMessage(ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, Outcome outcome, bool batchable,
            TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            this.ThrowIfClosed();
            var disposeResult = new DisposeAsyncResult(this, deliveryTag, txnId, outcome, batchable, timeout, cancellationToken, callback, state);
            return disposeResult;
        }

        void CancelPendingOperations(bool aborted, out Queue<AmqpMessage> messagesToRelease)
        {
            messagesToRelease = null;
            List<ReceiveAsyncResult> waiters = null;
            if (!this.messageQueue.IsEmpty)
            {
                messagesToRelease = new Queue<AmqpMessage>();
                while (this.messageQueue.TryDequeue(out AmqpMessage message))
                {
                    messagesToRelease.Enqueue(message);
                }
            }

            if (this.waiterManager.Count > 0)
            {
                waiters = this.waiterManager.RemoveAll();
            }

            if (waiters != null)
            {
                ActionItem.Schedule(static o =>
                {
                    var state = (Tuple<List<ReceiveAsyncResult>, bool, Exception>)o;
                    List<ReceiveAsyncResult> waitersToCancel = state.Item1;
                    foreach (ReceiveAsyncResult waiter in waitersToCancel)
                    {
                        if (state.Item2)
                        {
                            waiter.Signal(false, new OperationCanceledException("Link aborted", state.Item3));
                        }
                        else
                        {
                            waiter.Signal(false, null);
                        }
                    }
                },
                new Tuple<List<ReceiveAsyncResult>, bool, Exception>(waiters, aborted, this.TerminalException));
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
                AmqpTrace.Provider.AmqpCacheMessage(this, message.DeliveryId.Value, message.Segments, this.Settings.TotalLinkCredit, this.LinkCredit);
                this.prefetchSizeTracker?.Track(message);
                this.messageQueue.Enqueue(message);
                this.CheckWaiter();
            }
        }

        void CheckWaiter()
        {
            if (Interlocked.Increment(ref this.checkWaiterCount) == 1)
            {
                ActionItem.Schedule(static o => CheckCallback(o), this);
            }
        }

        static void CheckCallback(object state)
        {
            var thisPtr = (ReceivingAmqpLink)state;
            try
            {
                int count = 1;
                do
                {
                    thisPtr.DoCheckWaiter();
                    count = Interlocked.Add(ref thisPtr.checkWaiterCount, -count);
                }
                while (count > 0);
            }
            catch (Exception exception)
            {
                thisPtr.SafeClose(exception);
            }
        }

        // This should be the only place where waiters' requests are fulfilled.
        void DoCheckWaiter()
        {
            if (!this.Settings.AutoSendFlow)
            {
                bool sendFlow = false;
                lock (this.SyncRoot)
                {
                    int count = this.waiterManager.ResetRequestCount();
                    if (count > 0)
                    {
                        this.LinkCredit += (uint)count;
                        sendFlow = true;
                    }
                }

                if (sendFlow)
                {
                    this.SendFlow(false);
                }
            }

            if (this.messageQueue.IsEmpty)
            {
                return;
            }

            ReceiveAsyncResult waiter = this.waiterManager.PeekWaiter();
            while (waiter != null && this.messageQueue.TryPeek(out AmqpMessage message))
            {
                if (waiter.TryAdd(message))
                {
                    this.messageQueue.TryDequeue(out _);
                    if (waiter.MessageCount >= waiter.RequestedMessageCount)
                    {
                        waiter.Signal(false, null);
                        waiter = this.waiterManager.PeekWaiter();
                    }
                }
                else
                {
                    waiter = this.waiterManager.PeekWaiter();
                }
            }
        }

        readonly struct ReceiveParam
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

        readonly struct DisposeParam
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

        sealed class PrefetchSizeTracker
        {
            const uint MinCredit = 10;
            const uint MaxCredit = 10000;
            const ulong CreditUpdateInterval = 50;
            const ulong ResetThreshold = ulong.MaxValue - MaxCredit * 100 * 1024;
            readonly ReceivingAmqpLink parent;
            readonly ulong totalCacheSize;
            ulong totalMessageSize;
            ulong totalMessageCount;
            uint credit;

            public PrefetchSizeTracker(ReceivingAmqpLink parent, long totalCacheSize)
            {
                this.parent = parent;
                this.totalCacheSize = (ulong)totalCacheSize;
                this.credit = MinCredit;   // start small
            }

            public void Track(AmqpMessage message)
            {
                this.totalMessageSize += (ulong)message.SerializedMessageSize;
                this.totalMessageCount++;
                if (this.totalMessageCount % CreditUpdateInterval == 0)
                {
                    this.Update();
                }
            }

            public void Update()
            {
                ulong size = this.totalMessageSize;
                ulong count = this.totalMessageCount;
                bool sendFlow = false;
                if (size > 0 && count > 0)
                {
                    ulong average = size / count;
                    uint result = (uint)(this.totalCacheSize / average);
                    if (result < MinCredit)
                    {
                        result = MinCredit;
                    }
                    else if (result > MaxCredit)
                    {
                        result = MaxCredit;
                    }
                    else
                    {
                        // make it multiples of min to reduce flows
                        result = result / MinCredit * MinCredit;
                    }

                    sendFlow = result > this.credit;
                    this.credit = result;

                    if (size >= ResetThreshold)
                    {
                        // Do not reset to 0. Try to keep the learnt average.
                        this.totalMessageCount = MaxCredit;
                        this.totalMessageSize = MaxCredit * average;
                    }
                }

                lock (this.parent.SyncRoot)
                {
                    this.parent.Settings.TotalLinkCredit = this.credit;
                    this.parent.LinkCredit = this.credit;
                }

                if (sendFlow)
                {
                    this.parent.SendFlow(false);
                }
            }
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
            int state;  // 0: active idle, 1: busy adding, 2: completed
            List<AmqpMessage> messages;

            public ReceiveAsyncResult(ReceivingAmqpLink parent, int requestedMessageCount,
                TimeSpan batchWaitTimeout, TimeSpan timeout, AsyncCallback callback, object state)
                : base(callback, state)
            {
                this.parent = parent;
                this.batchWaitTimeout = batchWaitTimeout;
                this.requestedMessageCount = requestedMessageCount;
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

            public LinkedListNode<ReceiveAsyncResult> Node
            {
                get { return this.node; }
            }

            public static bool End(IAsyncResult result, out List<AmqpMessage> messages)
            {
                ReceiveAsyncResult thisPtr = AsyncResult.End<ReceiveAsyncResult>(result);
                messages = thisPtr.messages ?? AmqpConstants.EmptyMessages;
                return messages.Count > 0;
            }

            public void Initialize(LinkedListNode<ReceiveAsyncResult> node, CancellationToken cancellationToken)
            {
                this.node = node;
                if (this.timeout != TimeSpan.MaxValue && this.timeout != Timeout.InfiniteTimeSpan)
                {
                    this.timer = new Timer(static s => OnTimer(s), this, this.timeout, Timeout.InfiniteTimeSpan);
                }

                if (cancellationToken.CanBeCanceled)
                {
                    this.cancellationTokenRegistration = cancellationToken.Register(o =>
                    {
                        ReceiveAsyncResult result = (ReceiveAsyncResult)o;
                        result.Cancel();
                    }, this);
                }
            }

            public void Cancel()
            {
                this.Signal(false, new TaskCanceledException());
            }

            public bool TryAdd(AmqpMessage message)
            {
                if (Interlocked.CompareExchange(ref this.state, 1, 0) == 0)
                {
                    try
                    {
                        if (this.messages == null)
                        {
                            this.messages = new List<AmqpMessage>(Math.Min(8, this.requestedMessageCount));
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

                        return true;
                    }
                    finally
                    {
                        Interlocked.Exchange(ref this.state, 0);
                    }
                }

                return false;
            }

            public void OnRemoved()
            {
                this.node = null;
            }

            public void Signal(bool syncComplete, Exception exception)
            {
                int code = Interlocked.CompareExchange(ref this.state, 2, 0);
                if (code == 0)
                {
                    this.CompleteInternal(syncComplete, exception);
                }
                else if (code == 1)
                {
                    try
                    {
                        this.timer.Change(TimeSpan.FromMilliseconds(20), Timeout.InfiniteTimeSpan);
                    }
                    catch (ObjectDisposedException)
                    {
                    }
                }
            }

            void CompleteInternal(bool syncComplete, Exception exception)
            {
                this.timer?.Dispose();
                this.cancellationTokenRegistration.Dispose();   // No-op if not registered.
                if (this.node != null)
                {
                    this.parent.waiterManager.RemoveWaiter(this);
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

            static void OnTimer(object state)
            {
                ReceiveAsyncResult thisPtr = (ReceiveAsyncResult)state;
                thisPtr.Signal(false, null);
            }
        }

        sealed class WaiterManager : LinkedList<ReceiveAsyncResult>
        {
            readonly object syncRoot;
            readonly ReceivingAmqpLink link;
            int totalCount;
            int leftoverCount;

            public WaiterManager(ReceivingAmqpLink link)
            {
                this.syncRoot = new object();
                this.link = link;
            }

            public int ResetRequestCount()
            {
                lock (this.syncRoot)
                {
                    int count = this.totalCount;
                    this.totalCount = 0;
                    return count;
                }
            }

            public void AddWaiter(ReceiveAsyncResult waiter, CancellationToken cancellationToken)
            {
                var node = new LinkedListNode<ReceiveAsyncResult>(waiter);
                lock (this.syncRoot)
                {
                    this.AddLast(node);
                    int count = Math.Min(waiter.RequestedMessageCount, this.leftoverCount);
                    this.leftoverCount -= count;
                    this.totalCount += waiter.RequestedMessageCount - count;
                }

                waiter.Initialize(node, cancellationToken);
            }

            public bool RemoveWaiter(ReceiveAsyncResult waiter)
            {
                lock (this.syncRoot)
                {
                    if (waiter.Node == null)
                    {
                        return false;
                    }

                    this.Remove(waiter.Node);
                    waiter.OnRemoved();
                    this.leftoverCount += waiter.RequestedMessageCount - waiter.MessageCount;
                    return true;
                }
            }

            public ReceiveAsyncResult PeekWaiter()
            {
                lock (this.syncRoot)
                {
                    return this.First?.Value;
                }
            }

            public List<ReceiveAsyncResult> RemoveAll()
            {
                var list = new List<ReceiveAsyncResult>(this.Count);
                lock (this.syncRoot)
                {
                    foreach (var waiter in this)
                    {
                        waiter.OnRemoved();
                        list.Add(waiter);
                    }

                    this.Clear();
                    return list;
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
    }
}
