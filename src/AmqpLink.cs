// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// Implements the AMQP 1.0 link.
    /// </summary>
    public abstract class AmqpLink : AmqpObject, IWorkDelegate<Delivery>
    {
        static readonly AsyncCallback onProviderLinkOpened = OnProviderLinkOpened;
        readonly object syncRoot;
        readonly AmqpLinkSettings settings;
        readonly Outcome defaultOutcome;
        readonly SerializedWorker<Delivery> inflightDeliveries; // has link credit, may need session credit
        Action<uint, bool, ArraySegment<byte>> creditListener;

        // flow control state
        SequenceNumber deliveryCount;
        uint available;
        uint linkCredit;
        bool drain;
        uint needFlowCount;
        int sendingFlow;
        uint? tempTotalCredit;
        uint bufferedCredit;
        bool sessionWindowClosed;
        int references; // make sure no frames are sent after close

        /// <summary>
        /// Initializes the link object.
        /// </summary>
        /// <param name="session">The session in which the link is created.</param>
        /// <param name="linkSettings">The link settings.</param>
        protected AmqpLink(AmqpSession session, AmqpLinkSettings linkSettings)
            : this("link", session, linkSettings)
        {
        }

        /// <summary>
        /// Initializes the link object.
        /// </summary>
        /// <param name="type">A prefix to the link name for debugging purposes.</param>
        /// <param name="session">The session in which the link is created.</param>
        /// <param name="linkSettings">The link settings.</param>
        protected AmqpLink(string type, AmqpSession session, AmqpLinkSettings linkSettings)
            : base(type)
        {
            this.references = 1;
            this.syncRoot = new object();
            this.settings = linkSettings ?? throw new ArgumentNullException(nameof(linkSettings));
            this.linkCredit = this.settings.TotalLinkCredit;

            Source source = (Source)this.settings.Source;
            if (source != null)
            {
                this.defaultOutcome = source.DefaultOutcome;
            }

            if (this.defaultOutcome == null)
            {
                this.defaultOutcome = AmqpConstants.ReleasedOutcome;
            }

            this.UnsettledMap = new Dictionary<ArraySegment<byte>, Delivery>(ByteArrayComparer.Instance);
            if (session != null)
            {
                this.AttachTo(session);
            }

            if (!linkSettings.IsReceiver())
            {
                this.inflightDeliveries = new SerializedWorker<Delivery>(this);
            }
        }

        /// <summary>
        /// A handler to handle custom properties received in a flow command.
        /// </summary>
        public event EventHandler PropertyReceived;

        /// <summary>
        /// Gets the link name.
        /// </summary>
        public string Name
        {
            get { return this.settings.LinkName; }
        }

        /// <summary>
        /// Gets the link local handle.
        /// </summary>
        public uint? LocalHandle
        {
            get { return this.settings.Handle; }
            set { this.settings.Handle = value; }
        }

        /// <summary>
        /// Gets the link remote handle.
        /// </summary>
        public uint? RemoteHandle
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the session in which the link is created.
        /// </summary>
        public AmqpSession Session
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the link settings.
        /// </summary>
        public AmqpLinkSettings Settings
        {
            get { return this.settings; }
        }

        /// <summary>
        /// Gets the role of the link. True if it is a receiver; false if it is a sender.
        /// </summary>
        public bool IsReceiver
        {
            get
            {
                return this.settings.Role.Value;
            }
        }

        /// <summary>
        /// Gets the current credit.
        /// </summary>
        public uint LinkCredit
        {
            get
            {
                return this.linkCredit;
            }

            protected set
            {
                this.linkCredit = value;
            }
        }

        /// <summary>
        /// Gets the available deliveries from the link endpoint. May not always be set.
        /// </summary>
        public virtual uint Available
        {
            get
            {
                return this.available;
            }
        }

        /// <summary>
        /// Gets or sets the maximum frame size of the underlying connection.
        /// </summary>
        public uint MaxFrameSize
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the map of unsettled deliveries.
        /// </summary>
        public IDictionary<ArraySegment<byte>, Delivery> UnsettledMap { get; }

        /// <summary>
        /// Gets an object that synchronizes access to shared link endpoint state.
        /// </summary>
        protected object SyncRoot
        {
            get
            {
                return this.syncRoot;
            }
        }

        /// <summary>
        /// Gets the drain state of the link endpoint.
        /// </summary>
        public bool Drain => this.drain;

        /// <summary>
        /// Return the <see cref="AmqpLinkIdentifier"/> for this link.
        /// </summary>
        public AmqpLinkIdentifier LinkIdentifier { get; private set; }

        internal override TimeSpan OperationTimeout
        {
            get
            {
                return this.settings.OperationTimeoutInternal == default ?
                    this.Session.Connection.OperationTimeout :
                    this.settings.OperationTimeoutInternal;
            }
        }

        internal AmqpLinkTerminus Terminus { get; set; }

        internal bool IsRecoverable
        {
            get
            {
                if (this.Session.Connection.LinkRecoveryEnabled)
                {
                    AmqpSymbol expiryPolicy = this.Settings.GetExpiryPolicy();
                    return expiryPolicy.Equals(TerminusExpiryPolicy.LinkDetach) ||
                        expiryPolicy.Equals(TerminusExpiryPolicy.SessionEnd) ||
                        expiryPolicy.Equals(TerminusExpiryPolicy.ConnectionClose) ||
                        expiryPolicy.Equals(TerminusExpiryPolicy.Never);
                }

                return false;
            }
        }

        /// <summary>
        /// Attaches the link to a session.
        /// </summary>
        /// <param name="session">The session.</param>
        public void AttachTo(AmqpSession session)
        {
            Fx.Assert(this.Session == null, "The link is already attached to a session");
            this.MaxFrameSize = session.Connection.Settings.MaxFrameSize();
            this.Session = session;
            this.LinkIdentifier = new AmqpLinkIdentifier(this.Name, this.settings.IsReceiver(), this.Session.Connection.Settings.ContainerId);
            session.AttachLink(this);
        }

        /// <summary>
        /// Registers a callback which is invoked when credits are updated by received flow commands.
        /// </summary>
        /// <param name="creditListener">The callback that is invoked with credit, drain and txn-id arguments.</param>
        public void RegisterCreditListener(Action<uint, bool, ArraySegment<byte>> creditListener)
        {
            lock (this.syncRoot)
            {
                if (this.creditListener != null)
                {
                    throw new InvalidOperationException("Credit listener already registered.");
                }

                this.creditListener = creditListener;
            }
        }

        /// <summary>
        /// Drains the credit to stop remote peer transferring more deliveries.
        /// </summary>
        public void DrainCredits()
        {
            lock (this.syncRoot)
            {
                this.deliveryCount += (int)this.linkCredit;
                this.linkCredit = 0;
                if (!this.IsReceiver)
                {
                    this.SendFlow(false, true, null);
                }
            }
        }

        /// <summary>
        /// For internal implementation.
        /// </summary>
        /// <param name="frame">The received frame.</param>
        public void ProcessFrame(Frame frame)
        {
            Performative command = frame.Command;

            try
            {
                if (command.DescriptorCode == Attach.Code)
                {
                    this.OnReceiveAttach((Attach)command);
                }
                else if (command.DescriptorCode == Detach.Code)
                {
                    this.OnReceiveDetach((Detach)command);
                }
                else if (command.DescriptorCode == Transfer.Code)
                {
                    this.OnReceiveTransfer((Transfer)command, frame);
                }
                else if (command.DescriptorCode == Flow.Code)
                {
                    this.OnReceiveFlow((Flow)command);
                }
                else
                {
                    throw new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpInvalidPerformativeCode, command.DescriptorCode));
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpTrace.Provider.AmqpLogError(this, nameof(ProcessFrame), exception);
                this.SafeClose(exception);
            }
        }

        /// <summary>
        /// Create either a <see cref="SendingAmqpLink"/> or a <see cref="ReceivingAmqpLink"/> with the provided link settings,
        /// depending on its Role property.
        /// </summary>
        /// <param name="linkSettings">The link settings used to create a new link.</param>
        /// <returns>A new link to be created.</returns>
        internal static AmqpLink Create(AmqpLinkSettings linkSettings)
        {
            AmqpLink link;
            if (linkSettings.Role == null)
            {
                throw new ArgumentNullException(nameof(linkSettings.Role));
            }

            if (!linkSettings.Role.Value)
            {
                link = new SendingAmqpLink(linkSettings);
            }
            else
            {
                link = new ReceivingAmqpLink(linkSettings);
            }

            return link;
        }

        internal void OnFlow(Flow flow)
        {
            this.OnReceiveFlow(flow);
        }

        /// <summary>
        /// Attempts to send a delivery over the link.
        /// </summary>
        /// <param name="delivery">The delivery to send.</param>
        /// <returns>True if there is credit to transfer the delivery; false otherwise.</returns>
        public bool TrySendDelivery(Delivery delivery)
        {
            Fx.Assert(delivery.DeliveryTag.Array != null, "delivery-tag must be set.");

            // check link credit first
            bool canSend = false;
            lock (this.syncRoot)
            {
                if (this.linkCredit > 0)
                {
                    canSend = true;
                    this.deliveryCount.Increment();
                    if (this.linkCredit != uint.MaxValue)
                    {
                        --this.linkCredit;
                    }
                }
            }

            if (!canSend)
            {
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Send, "NoCredit");
                return false;
            }

            this.StartSendDelivery(delivery);
            return true;
        }

        /// <summary>
        /// Sends the delivery even if there is no credit left. It may happen that the credits are reduced
        /// to 0 while messages become available in the link endpoint.
        /// </summary>
        /// <param name="delivery">The delivery to send.</param>
        public void ForceSendDelivery(Delivery delivery)
        {
            // Send the delivery even if there is no link credit
            lock (this.syncRoot)
            {
                this.deliveryCount.Increment();
                if (this.linkCredit > 0 && this.linkCredit < uint.MaxValue)
                {
                    --this.linkCredit;
                }
            }

            this.StartSendDelivery(delivery);
        }

        /// <summary>
        /// Updates the state of a delivery.
        /// </summary>
        /// <param name="delivery">The delivery to update.</param>
        /// <param name="settled">True if the delivery is settled; false otherwise.</param>
        /// <param name="state">The state of the delivery.</param>
        public void DisposeDelivery(Delivery delivery, bool settled, DeliveryState state)
        {
            this.DisposeDelivery(delivery, settled, state, false);
        }

        /// <summary>
        /// Updates the state of a delivery.
        /// </summary>
        /// <param name="delivery">The delivery to update.</param>
        /// <param name="settled">True if the delivery is settled; false otherwise.</param>
        /// <param name="state">The state of the delivery.</param>
        /// <param name="noFlush">True to not send a disposition frame right away.</param>
        public void DisposeDelivery(Delivery delivery, bool settled, DeliveryState state, bool noFlush)
        {
            this.DoActionIfNotClosed(
                static (thisPtr, paramDelivery, paramSettled, paramState, paramNoFlush) =>
                {
                    thisPtr.DisposeDeliveryInternal(paramDelivery, paramSettled, paramState, paramNoFlush);
                    return true;
                },
                delivery,
                settled,
                state,
                noFlush);
        }

        /// <summary>
        /// Updates the state of a delivery using its delivery tag.
        /// </summary>
        /// <param name="deliveryTag">The delivery tag that identifies the delivery.</param>
        /// <param name="settled">True if the delivery is settled; false otherwise.</param>
        /// <param name="state">The state of the delivery.</param>
        /// <param name="batchable">True if the state change can be batched with other deliveries.</param>
        /// <returns>True if the delivery is found and updated; false otherwise.</returns>
        public bool DisposeDelivery(ArraySegment<byte> deliveryTag, bool settled, DeliveryState state, bool batchable)
        {
            Delivery delivery = null;
            bool result = false;
            lock (this.syncRoot)
            {
                result = this.UnsettledMap.TryGetValue(deliveryTag, out delivery);
            }

            if (result)
            {
                delivery.Batchable = batchable;
                this.DisposeDelivery(delivery, settled, state);
                return true;
            }
            else
            {
                AmqpTrace.Provider.AmqpDeliveryNotFound(this, Extensions.GetString(deliveryTag));
                return false;
            }
        }

        /// <summary>
        /// Sets a new credit on the link. A flow frame may be sent to the remote peer.
        /// It should be called only for a receiver link endpoint.
        /// </summary>
        /// <param name="totalCredit">The new credit.</param>
        /// <param name="applyNow">True to update link flow control state right away and send a flow.</param>
        /// <param name="updateAutoFlow">To set <see cref="AmqpLinkSettings.AutoSendFlow"/> to true if totalCredit is greater than 0.</param>
        /// <remarks>The credit update is not a direct assignment. This method accounts for inflight
        /// deliveries in case credits are reduced.</remarks>
        public void SetTotalLinkCredit(uint totalCredit, bool applyNow, bool updateAutoFlow = false)
        {
            Fx.Assert(this.IsReceiver, "Only receiver can change the total link credit.");
            lock (this.syncRoot)
            {
                bool needFlowUpdate =
                    this.tempTotalCredit == null &&
                    this.linkCredit == 0 &&
                    totalCredit > 0;

                this.tempTotalCredit = totalCredit;
                if (updateAutoFlow)
                {
                    this.settings.AutoSendFlow = totalCredit > 0;
                }

                if ((applyNow || needFlowUpdate) &&
                    this.ApplyTempTotalLinkCredit() &&
                    this.State == AmqpObjectState.Opened)
                {
                    this.SendFlow(false, false, properties: null);
                }
            }
        }

        /// <summary>
        /// When <see cref="AmqpLinkSettings.AutoSendFlow"/> is false, sets the link credit
        /// and sends a flow frame.
        /// </summary>
        /// <param name="credit">The link credit.</param>
        /// <param name="drain">The drain flag.</param>
        /// <param name="txnId">The transaction-id for transfers allowed by the credits.</param>
        /// <remarks>This method should be called when application needs full control of
        /// the link flow control.</remarks>
        public void IssueCredit(uint credit, bool drain, ArraySegment<byte> txnId)
        {
            if (!this.settings.AutoSendFlow)
            {
                lock (this.syncRoot)
                {
                    this.settings.TotalLinkCredit = credit;
                    this.linkCredit = credit;
                }

                this.SendFlow(false, drain, txnId);
            }
        }

        /// <summary>
        /// Decide if the current link should be allowed to steal a link endpoint with the provided <see cref="AmqpLinkSettings"/>.
        /// </summary>
        /// <param name="amqpLinkSettings">The linkSettings for the existing link that is about to be stolen.</param>
        /// <returns>True if link stealing this link should be allowed.</returns>
        internal protected virtual bool AllowLinkStealing(AmqpLinkSettings amqpLinkSettings)
        {
            return true;
        }

        internal void NotifySessionCredit(int credit)
        {
            if (this.inflightDeliveries != null)
            {
                this.inflightDeliveries.ContinueWork();
            }
            else
            {
                this.OnCreditAvailable(credit, 0, false, AmqpConstants.NullBinary);
            }
        }

        // bottom-up: from session disposition to link to application
        internal void OnDisposeDelivery(Delivery delivery)
        {
            if (delivery.Settled)
            {
                lock (this.syncRoot)
                {
                    this.UnsettledMap.Remove(delivery.DeliveryTag);
                }

                this.RemoveUnsettledDeliveryFromTerminusStoreIfNeeded(delivery.DeliveryTag);
                this.OnDeliverySettled();
            }

            this.OnDisposeDeliveryInternal(delivery);

            // in case the receiver does not honor the settlement policy
            if (!this.IsReceiver && 
                this.settings.SettleType != SettleMode.SettleOnDispose && 
                !delivery.Settled &&
                !delivery.State.Transactional())
            {
                this.CompleteDelivery(delivery.DeliveryTag);
            }
        }

        internal void CompleteDelivery(ArraySegment<byte> deliveryTag)
        {
            Delivery delivery = null;
            bool result = false;
            lock (this.syncRoot)
            {
                result = this.UnsettledMap.TryGetValue(deliveryTag, out delivery);
            }

            if (result) 
            {
                this.DisposeDelivery(delivery, true, delivery.State);
            }
        }

        /// <summary>
        /// Sends a map in a flow command to the remote peer.
        /// </summary>
        /// <param name="properties">A map containing the properties.</param>
        /// <remarks>Enables application to build a simple communication channel
        /// using the existing link. See <see cref="PropertyReceived"/>.</remarks>
        public void SendProperties(Fields properties)
        {
            this.SendFlow(false, false, properties);
        }

        /// <summary>
        /// Processes a received flow command.
        /// </summary>
        /// <param name="flow">The received flow command.</param>
        /// <returns>More credits allowed by the received flow command.</returns>
        public uint ProcessFlow(Flow flow)
        {
            uint moreCredit = 0;
            if (flow.Properties != null)
            {
                EventHandler propertyHandler = this.PropertyReceived;
                if (propertyHandler != null)
                {
                    propertyHandler(flow.Properties, EventArgs.Empty);
                    return moreCredit;
                }
            }

            uint flowLinkCredit = flow.LinkCredit();
            lock (this.syncRoot)
            {
                this.drain = flow.Drain ?? false;
                if (this.IsReceiver)
                {
                    this.available = flow.Available ?? uint.MaxValue;
                    if (this.drain)
                    {
                        this.DrainCredits();
                    }

                    this.ApplyTempTotalLinkCredit();
                }
                else
                {
                    if (flowLinkCredit != uint.MaxValue)
                    {
                        if (this.linkCredit == uint.MaxValue)
                        {
                            this.linkCredit = flowLinkCredit;
                            moreCredit = 0;
                        }
                        else
                        {
                            SequenceNumber otherDeliveryLimit = (flow.DeliveryCount ?? 0u) + flowLinkCredit;
                            SequenceNumber thisDeliveryLimit = this.deliveryCount.Value + this.linkCredit;
                            int delta = otherDeliveryLimit - thisDeliveryLimit;
                            if (delta > 0)
                            {
                                this.linkCredit += (uint)delta;
                                moreCredit = (uint)delta;
                            }
                            else if (delta < 0)
                            {
                                uint reduced = (uint)(-delta);
                                this.linkCredit = reduced > this.linkCredit ? 0 : this.linkCredit - reduced;
                            }
                        }
                    }
                    else
                    {
                        this.linkCredit = uint.MaxValue;
                        moreCredit = uint.MaxValue;
                    }
                }
            }

            if (moreCredit > 0 || this.drain)
            {
                ArraySegment<byte> txnId = this.GetTxnIdFromFlow(flow);
                this.OnCreditAvailable(0, moreCredit, this.drain, txnId);
                if (this.linkCredit > 0 || this.drain)
                {
                    this.creditListener?.Invoke(moreCredit, this.drain, txnId);
                }
            }

            if (flow.Echo())
            {
                this.SendFlow(false, false, properties: null);
            }

            return moreCredit;
        }

        /// <summary>
        /// Creates a delivery from a received transfer command.
        /// </summary>
        /// <param name="transfer">The received transfer command.</param>
        /// <param name="delivery">The delivery.</param>
        /// <returns>True if the transfer is for a new delivery; false if it is
        /// for the existing delivery.</returns>
        protected abstract bool CreateDelivery(Transfer transfer, out Delivery delivery);

        internal virtual void OnIoEvent(IoEvent ioEvent)
        {
            EventHandler temp = this.PropertyReceived;
            if (temp != null)
            {
                AmqpTrace.Provider.AmqpIoEvent(this, ioEvent, (long)this.linkCredit);

                Fields properties = new Fields();
                properties.Add(AmqpConstants.IoEvent, (int)ioEvent);
                temp(properties, EventArgs.Empty);
            }
        }

        bool IWorkDelegate<Delivery>.Invoke(Delivery delivery)
        {
            return this.DoActionIfNotClosed(
                static (thisPtr, paramDelivery, p1, p2, p3) =>
                {
                    bool sent = thisPtr.SendDelivery(paramDelivery);
                    if (!sent)
                    {
                        thisPtr.sessionWindowClosed = true;
                        thisPtr.OnIoEvent(IoEvent.WriteBufferQueueFull);
                    }
                    else if (thisPtr.sessionWindowClosed)
                    {
                        thisPtr.sessionWindowClosed = true;
                        thisPtr.OnIoEvent(IoEvent.WriteBufferQueueEmpty);
                    }

                    return sent;
                },
                delivery,
                0,
                0,
                0);
        }

        /// <summary>
        /// Opens the link.
        /// </summary>
        /// <returns>True if the link is open; false if open is pending.</returns>
        protected override bool OpenInternal()
        {
            AmqpObjectState state = this.SendAttach(this.settings);
            if (this.IsReceiver)
            {
                lock (this.syncRoot)
                {
                    this.ApplyTempTotalLinkCredit();
                }
            }

            return state == AmqpObjectState.Opened;
        }

        /// <summary>
        /// Closes the link.
        /// </summary>
        /// <returns>True if the link is closed; false if close is pending.</returns>
        /// <remarks>All inflight deliveries are canceled.</remarks>
        protected override bool CloseInternal()
        {
            AmqpObjectState state = this.State;
            if (state == AmqpObjectState.OpenReceived ||
                state == AmqpObjectState.ClosePipe)
            {
                this.SendAttach(this.CreateAttachForClose());
            }

            if (this.inflightDeliveries != null)
            {
                this.inflightDeliveries.Abort();
            }

            if (Interlocked.Decrement(ref this.references) > 0)
            {
                // let other thread finish its work. close is low pri
                SpinWait.SpinUntil(() => this.references <= 0, 10000);
            }

            this.Session.Flush();

            if (StateTransition.CanTransite(state, StateTransition.SendClose))
            {
                state = this.SendDetach();
            }
            else
            {
                state = this.State = AmqpObjectState.End;
            }

            return state == AmqpObjectState.End;
        }

        /// <summary>
        /// Aborts the link object. All in flight deliveries are canceled.
        /// </summary>
        protected override void AbortInternal()
        {
            Interlocked.Decrement(ref this.references);

            if (this.inflightDeliveries != null)
            {
                this.inflightDeliveries.Abort();
            }
        }

        /// <summary>
        /// Sets a flow on this link.
        /// </summary>
        /// <param name="echo"><see cref="Flow.Echo"/></param>
        protected void SendFlow(bool echo)
        {
            this.SendFlow(echo, false, properties: null);
        }

        void ProcessTransfer(Transfer transfer, Frame rawFrame, Delivery delivery, bool newDelivery)
        {
            if (newDelivery)
            {
                if (this.linkCredit < uint.MaxValue)
                {
                    bool creditAvailable = true;
                    lock (this.syncRoot)
                    {
                        // Updating new total credit is delayed to avoid flooding
                        // the network with flows when there are many links that are
                        // frequently opened/closed
                        if (this.tempTotalCredit != null &&
                            this.ApplyTempTotalLinkCredit())
                        {
                            this.SendFlow(false, false, properties: null);
                        }

                        if (this.linkCredit == 0 && this.bufferedCredit == 0)
                        {
                            creditAvailable = false;
                        }
                        else
                        {
                            this.deliveryCount.Increment();
                            if (this.bufferedCredit > 0)
                            {
                                --this.bufferedCredit;
                            }
                            else if (this.linkCredit < uint.MaxValue)
                            {
                                --this.linkCredit;
                            }
                        }
                    }

                    if (!creditAvailable)
                    {
                        throw new AmqpException(AmqpErrorCode.TransferLimitExceeded,
                            AmqpResources.GetString(AmqpResources.AmqpTransferLimitExceeded, delivery.DeliveryId.Value));
                    }
                }

                if (!delivery.Settled && !delivery.Aborted)
                {
                    lock (this.syncRoot)
                    {
                        this.UnsettledMap.Add(delivery.DeliveryTag, delivery);
                    }
                }
            }

            this.OnProcessTransfer(delivery, transfer, rawFrame);
        }

        /// <summary>
        /// A <see cref="Flow"/> is received. Override this method to process the flow if needed.
        /// </summary>
        /// <param name="flow">The received flow performative.</param>
        protected virtual void OnReceiveFlow(Flow flow)
        {
            this.ProcessFlow(flow);
        }

        /// <summary>
        /// A method to override to process the received transfer performative.
        /// </summary>
        /// <param name="delivery">The delivery to which the transfer belong.</param>
        /// <param name="transfer">The received transfer.</param>
        /// <param name="rawFrame">The frame that contains the transfer.</param>
        protected abstract void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame rawFrame);

        /// <summary>
        /// A method to override to handle link flow event.
        /// </summary>
        /// <param name="session">The increased session window size.</param>
        /// <param name="link">The increased link credit.</param>
        /// <param name="drain"><see cref="Flow.Drain"/></param>
        /// <param name="txnId">The transaction identifier in the flow.</param>
        protected abstract void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId);

        /// <summary>
        /// A method to override to handle a delivery state update by the remote peer.
        /// </summary>
        /// <param name="delivery">The delivery whose state is updated.</param>
        protected abstract void OnDisposeDeliveryInternal(Delivery delivery);

        /// <summary>
        /// Process and consolidate the unsettled deliveries sent with the remote Attach frame, by checking against the unsettled deliveries for this link terminus.
        /// </summary>
        /// <param name="remoteAttach">The incoming Attach from remote which contains the remote's unsettled delivery states.</param>
        protected abstract void ProcessUnsettledDeliveries(Attach remoteAttach);

        internal bool SendDelivery(Delivery delivery)
        {
            bool settled = delivery.Settled;
            bool more = true;
            while (more)
            {
                bool firstTransfer = delivery.BytesTransfered == 0;
                Transfer transfer = new Transfer();
                transfer.Handle = this.LocalHandle;
                transfer.More = more;
                transfer.Aborted = delivery.Aborted;
                transfer.Resume = delivery.Resume;
                if (firstTransfer)
                {
                    transfer.DeliveryId = uint.MaxValue;    // reserve the space first
                    transfer.DeliveryTag = delivery.DeliveryTag;
                    transfer.MessageFormat = delivery.MessageFormat ?? AmqpConstants.AmqpMessageFormat;
                    transfer.Batchable = delivery.Batchable;
                    transfer.State = delivery.State;
                    if (settled)
                    {
                        transfer.Settled = true;
                    }

                    if (delivery.TxnId.Array != null)
                    {
                        transfer.State = new TransactionalState() { TxnId = delivery.TxnId };
                    }
                }

                ByteBuffer payload = null;
                if (delivery.Resume && delivery.State.IsTerminal())
                {
                    more = false;
                    transfer.More = false;
                }
                else
                {
                    uint maxFrameSize = this.MaxFrameSize == uint.MaxValue ? AmqpConstants.DefaultMaxFrameSize : this.MaxFrameSize;
                    int overhead = Frame.HeaderSize + transfer.EncodeSize;
                    if (overhead > maxFrameSize)
                    {
                        throw new AmqpException(AmqpErrorCode.FrameSizeTooSmall, null);
                    }

                    int payloadSize = (int)maxFrameSize - overhead;
                    payload = delivery.GetPayload(payloadSize, out more);
                    transfer.More = more;

                    if (payload == null)
                    {
                        if (firstTransfer)
                        {
                            throw new AmqpException(AmqpErrorCode.NotAllowed, AmqpResources.AmqpEmptyMessageNotAllowed);
                        }

                        Fx.Assert(!more, "More flag is set but a null payload is returned.");
                        break;
                    }
                }

                if (!this.Session.TrySendTransfer(firstTransfer ? delivery : null, transfer, payload))
                {
                    payload?.Dispose();
                    more = true;
                    break;
                }

                if (payload != null)
                {
                    delivery.CompletePayload(payload.Length);
                }
            }

            if (!more)
            {
                AmqpTrace.Provider.AmqpSentMessage(this, delivery.DeliveryId.Value, delivery.BytesTransfered);

                if (delivery.Settled)
                {
                    delivery.State = AmqpConstants.AcceptedOutcome;
                    this.OnDisposeDeliveryInternal(delivery);
                }
            }

            return !more;
        }

        // call this method if the operation should not be performed after close
        internal bool DoActionIfNotClosed<T1, T2, T3, T4>(Func<AmqpLink, T1, T2, T3, T4, bool> action, T1 p1, T2 p2, T3 p3, T4 p4)
        {
            int temp = Interlocked.Increment(ref this.references);
            try
            {
                if (temp > 1)
                {
                    return action(this, p1, p2, p3, p4);
                }
                else
                {
                    return false;
                }
            }
            finally
            {
                Interlocked.Decrement(ref this.references);
            }
        }

        void StartSendDelivery(Delivery delivery)
        {
            // The delivery may already be determined to be settled in scenarios such as link recovery.
            delivery.Settled = delivery.Settled || this.settings.SettleType == SettleMode.SettleOnSend;
            if (!delivery.Settled)
            {
                this.AddOrUpdateUnsettledDelivery(delivery);
            }

            delivery.Link = this;
            this.inflightDeliveries.DoWork(delivery);
        }

        internal void OnLinkStolen(bool shouldAbort)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, shouldAbort ? TraceOperation.Abort : TraceOperation.Close, "LinkStealing");

            if (shouldAbort)
            {
                this.TerminalException = new AmqpException(AmqpErrorCode.Stolen, AmqpResources.GetString(AmqpResources.AmqpLinkStolen, this.LinkIdentifier));
                this.Abort();
            }
            else
            {
                this.SafeClose(new AmqpException(AmqpErrorCode.Stolen, AmqpResources.GetString(AmqpResources.AmqpLinkStolen, this.LinkIdentifier)));
            }
        }

        internal void AddOrUpdateUnsettledDelivery(Delivery delivery)
        {
            lock (this.syncRoot)
            {
                if (this.UnsettledMap.ContainsKey(delivery.DeliveryTag))
                {
                    this.UnsettledMap.Remove(delivery.DeliveryTag);
                }

                this.UnsettledMap.Add(delivery.DeliveryTag, delivery);
            }
        }

        internal void RemoveUnsettledDeliveryFromTerminusStoreIfNeeded(ArraySegment<byte> deliveryTag)
        {
            if (this.Session.Connection.AmqpSettings.RuntimeProvider is ILinkRecoveryRuntimeProvider linkRecoveryRuntimeProvider)
            {
                linkRecoveryRuntimeProvider.TerminusStore?.TryRemoveDeliveryAsync(this.Terminus, deliveryTag).GetAwaiter().GetResult();
            }
        }

        void DisposeDeliveryInternal(Delivery delivery, bool settled, DeliveryState state, bool noFlush)
        {
            AmqpTrace.Provider.AmqpDispose(this, delivery.DeliveryId.Value, settled, state);
            if (settled && !delivery.Settled)
            {
                lock (this.syncRoot)
                {
                    this.UnsettledMap.Remove(delivery.DeliveryTag);
                }

                this.RemoveUnsettledDeliveryFromTerminusStoreIfNeeded(delivery.DeliveryTag);
            }

            this.Session.DisposeDelivery(this, delivery, settled, state, noFlush);

            if (delivery.Settled)
            {
                this.OnDeliverySettled();
            }
        }

        void OnDeliverySettled()
        {
            if (this.IsReceiver && this.settings.AutoSendFlow && this.linkCredit < uint.MaxValue)
            {
                bool sendFlow = false;
                lock (this.syncRoot)
                {
                    if (this.linkCredit < this.settings.TotalLinkCredit)
                    {
                        ++this.linkCredit;
                    }

                    ++this.needFlowCount;
                    if (this.needFlowCount >= this.settings.FlowThreshold && !this.CloseCalled)
                    {
                        sendFlow = true;
                        this.needFlowCount = 0;
                    }
                }

                if (sendFlow)
                {
                    this.SendFlow(false, false, properties: null);
                }
            }
        }

        AmqpObjectState SendAttach(Attach attach)
        {
            StateTransition transition = this.TransitState("S:ATTACH", StateTransition.SendOpen);
            this.Session.SendCommand(attach);
            return transition.To;
        }

        AmqpObjectState SendDetach()
        {
            Fx.Assert(this.LocalHandle != null, "The local handle should not be null when sending detach to remote, because it would mean that the link was not attached in the first place.");
            StateTransition transition = this.TransitState("S:DETACH", StateTransition.SendClose);

            Detach detach = new Detach();
            detach.Handle = this.LocalHandle;
            detach.Closed = true;
            Exception exception = this.TerminalException;
            if (exception != null && transition.To != AmqpObjectState.End)
            {
                detach.Error = Error.FromException(exception);
            }

            this.Session.SendCommand(detach);

            AmqpTrace.Provider.AmqpLinkDetach(this, this.Name, this.LocalHandle ?? 0u, "S:DETACH",
                detach.Error != null ? detach.Error.Condition.Value : string.Empty);

            return transition.To;
        }

        void OnReceiveAttach(Attach attach)
        {
            StateTransition stateTransition = this.TransitState("R:ATTACH", StateTransition.ReceiveOpen);

            Error error = this.Negotiate(attach);
            if (error != null)
            {
                this.OnLinkOpenFailed(new AmqpException(error));
                return;
            }

            if (stateTransition.From == AmqpObjectState.OpenSent)
            {
                if (this.IsReceiver)
                {
                    Source source = this.settings.Source as Source;
                    if (source != null && source.Dynamic())
                    {
                        source.Address = ((Source)attach.Source).Address;
                    }
                }
                else
                {
                    Target target = this.settings.Target as Target;
                    if (target != null && target.Dynamic())
                    {
                        target.Address = ((Target)attach.Target).Address;
                    }
                }

                if (attach.Properties != null)
                {
                    if (this.Settings.Properties == null)
                    {
                        this.settings.Properties = attach.Properties;
                    }
                    else
                    {
                        this.settings.Properties.Merge(attach.Properties);
                    }
                }
            }
            
            if (stateTransition.To == AmqpObjectState.Opened)
            {
                if ((this.IsReceiver && attach.Source == null) ||
                    (!this.IsReceiver && attach.Target == null))
                {
                    // not linkendpoint was created on the remote side
                    // a detach should be sent immediately by peer with error
                    return;
                }

                if (this.IsReceiver)
                {
                    this.deliveryCount = attach.InitialDeliveryCount.Value;
                    this.settings.Source = attach.Source;
                }
                else
                {
                    this.settings.Target = attach.Target;
                }

                // in some scenario (e.g. EventHub) the service
                // side can override the settle type based on
                // entity settings.
                this.settings.SettleType = attach.SettleType();
                this.CompleteOpen(false, null);
            }
            else if (stateTransition.To == AmqpObjectState.OpenReceived)
            {
                try
                {
                    this.Session.LinkFactory.BeginOpenLink(this, this.OperationTimeout, onProviderLinkOpened, this);
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    this.OnLinkOpenFailed(exception);
                }
            }
        }

        void OnReceiveDetach(Detach detach)
        {
            AmqpTrace.Provider.AmqpLinkDetach(this, this.Name, this.LocalHandle ?? 0u, "R:DETACH",
                detach.Error != null ? detach.Error.Condition.Value : string.Empty);

            this.OnReceiveCloseCommand("R:DETACH", detach.Error);
        }

        void OnReceiveTransfer(Transfer transfer, Frame rawFrame)
        {
            Delivery delivery = null;
            bool newDelivery = this.CreateDelivery(transfer, out delivery);

            // Initialize new delivery
            if (newDelivery)
            {
                delivery.Link = this;
                delivery.DeliveryId = transfer.DeliveryId.Value;
                delivery.DeliveryTag = transfer.DeliveryTag;
                delivery.Resume = transfer.Resume();
                delivery.Aborted = transfer.Aborted == true;
                delivery.Settled = transfer.Settled();
                delivery.Batchable = transfer.Batchable();
                delivery.MessageFormat = transfer.MessageFormat;
                delivery.State = transfer.State;
                TransactionalState txnState = transfer.State as TransactionalState;
                if (txnState != null)
                {
                    delivery.TxnId = txnState.TxnId;
                }
            }
            else if (delivery == null)
            {
                // if the upper layer wants to ignore this transfer, it must
                // return newDelivery=false and delivery=null
                return;
            }

            // let session process the transfer first
            if (!this.Session.OnAcceptTransfer(delivery, transfer, newDelivery))
            {
                return;
            }

            this.ProcessTransfer(transfer, rawFrame, delivery, newDelivery);
        }

        ArraySegment<byte> GetTxnIdFromFlow(Flow flow)
        {
            object txnId;
            if (flow.Properties != null &&
                (txnId = flow.Properties["txn-id"]) != null)
            {
                return (ArraySegment<byte>)txnId;
            }

            return AmqpConstants.NullBinary;
        }

        Error Negotiate(Attach attach)
        {
            Error error = null;
            try
            {
                if (this.IsRecoverable)
                {
                    this.ProcessUnsettledDeliveries(attach);
                }

                // TODO: need to negotiate the properties in case they are different between local and remote.
                // Honor the Source for sender and Target for Receiver.
                if (attach.MaxMessageSize() != 0)
                {
                    this.settings.MaxMessageSize = Math.Min(this.settings.MaxMessageSize(), attach.MaxMessageSize());
                }
            }
            catch (Exception e)
            {
                error = Error.FromException(e);
            }

            return error;
        }

        static void OnProviderLinkOpened(IAsyncResult result)
        {
            var thisPtr = (AmqpLink)result.AsyncState;
            AmqpTrace.Provider.AmqpLogOperationVerbose(thisPtr, TraceOperation.Execute, nameof(OnProviderLinkOpened));
            Exception openException = null;

            // Capture the exception from provider first
            try
            {
                thisPtr.Session.LinkFactory.EndOpenLink(result);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpTrace.Provider.AmqpLogError(thisPtr, "EndOpenLink", exception);
                openException = exception;
            }

            // Complete the link opening. This may fail if the link state changed already
            // while the provider is opening the link
            try
            {
                if (openException != null)
                {
                    thisPtr.OnLinkOpenFailed(openException);
                }
                else
                {
                    thisPtr.Open();
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpTrace.Provider.AmqpLogError(thisPtr, "CompleteOpenLink", exception);
                thisPtr.OnLinkOpenFailed(exception);
            }
        }

        void OnLinkOpenFailed(Exception exception)
        {
            if (this.State == AmqpObjectState.OpenReceived)
            {
                this.SendAttach(this.CreateAttachForClose());
            }

            this.SafeClose(exception);
        }

        bool ApplyTempTotalLinkCredit()
        {
            // Caller must hold the lock
            if (this.tempTotalCredit == null ||
                this.tempTotalCredit.Value == this.settings.TotalLinkCredit)
            {
                return false;
            }

            uint currentTotalCredit = this.settings.TotalLinkCredit;
            uint totalCredit = this.tempTotalCredit.Value;
            this.settings.TotalLinkCredit = totalCredit;
            this.tempTotalCredit = null;

            if (totalCredit == uint.MaxValue)
            {
                // No flow control
                this.linkCredit = totalCredit;
                this.bufferedCredit = 0;
            }
            else if (totalCredit > currentTotalCredit)
            {
                // Credit increased
                uint delta = totalCredit - currentTotalCredit;
                this.linkCredit += delta;
                this.bufferedCredit -= Math.Min(this.bufferedCredit, delta);
            }
            else if (currentTotalCredit < int.MaxValue)
            {
                // Credit reduced
                uint delta = currentTotalCredit - totalCredit;
                this.linkCredit -= Math.Min(this.linkCredit, delta);
                this.bufferedCredit += delta;
            }
            else
            {
                // No flow control to flow control
                // Link can be closed due to delivery overflow
                this.linkCredit = totalCredit;
                this.bufferedCredit = 0;
            }

            return true;
        }

        void SendFlow(bool echo, bool drain, ArraySegment<byte> txnId)
        {
            Fields properties = null;
            if (txnId.Array != null)
            {
                properties = new Fields();
                properties["txn-id"] = txnId;
            }

            this.SendFlow(echo, drain, properties);
        }

        void SendFlow(bool echo, bool drain, Fields properties)
        {
            this.DoActionIfNotClosed(
                (thisPtr, paramEcho, paramDrain, paramProperties, p5) =>
                {
                    thisPtr.SendFlowInternal(paramEcho, paramDrain, paramProperties);
                    return true;
                },
                echo,
                drain,
                properties,
                0);
        }

        void SendFlowInternal(bool echo, bool drain, Fields properties)
        {
            this.drain = drain;
            bool forceSend = echo || properties != null;

            if (!forceSend)
            {
                if (Interlocked.Increment(ref this.sendingFlow) != 1)
                {
                    return;
                }
            }

            do
            {
                Flow flow = new Flow();
                flow.Handle = this.LocalHandle;
                lock (this.syncRoot)
                {
                    flow.LinkCredit = this.linkCredit;
                    flow.Available = this.Available;
                    flow.DeliveryCount = this.deliveryCount.Value;
                    if (this.drain)
                    {
                        flow.Drain = true;
                    }
                }

                flow.Echo = echo;
                flow.Properties = properties;

                if (this.State == AmqpObjectState.Opened ||
                    this.State == AmqpObjectState.OpenSent ||
                    this.State == AmqpObjectState.OpenReceived)
                {
                    this.Session.SendFlow(flow);
                }
            }
            while (!forceSend && Interlocked.Decrement(ref this.sendingFlow) > 0);
        }

        Attach CreateAttachForClose()
        {
            Attach attach = new Attach();
            attach.LinkName = this.settings.LinkName;
            attach.Role = this.settings.Role;
            attach.Handle = this.settings.Handle;
            return attach;
        }
    }
}
