// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// Implements the transport-layer link, including
    /// link command handling and link flow control.
    /// </summary>
    public abstract class AmqpLink : AmqpObject, IWorkDelegate<Delivery>
    {
        static readonly AsyncCallback onProviderLinkOpened = OnProviderLinkOpened;
        readonly object syncRoot;
        readonly AmqpLinkSettings settings;
        readonly Outcome defaultOutcome;
        readonly Dictionary<ArraySegment<byte>, Delivery> unsettledMap;
        readonly SerializedWorker<Delivery> inflightDeliveries; // has link credit, may need session credit

        // flow control state
        SequenceNumber deliveryCount;
        uint available;
        uint linkCredit;
        bool drain;
        uint needFlowCount;
        int sendingFlow;
        uint? tempTotalCredit;
        uint bufferedCredit;
        int references; // make sure no frames are sent after close

        protected AmqpLink(AmqpSession session, AmqpLinkSettings linkSettings)
            : this("link", session, linkSettings)
        {
            this.inflightDeliveries = new SerializedWorker<Delivery>(this);
        }

        protected AmqpLink(string type, AmqpSession session, AmqpLinkSettings linkSettings)
            : base(type)
        {
            if (linkSettings == null)
            {
                throw new ArgumentNullException("linkSettings");
            }

            this.references = 1;
            this.syncRoot = new object();
            this.settings = linkSettings;
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

            this.unsettledMap = new Dictionary<ArraySegment<byte>, Delivery>(ByteArrayComparer.Instance);
            if (session != null)
            {
                this.AttachTo(session);
            }
        }

        internal event EventHandler PropertyReceived;

        public string Name
        {
            get { return this.settings.LinkName; }
        }

        public uint? LocalHandle
        {
            get { return this.settings.Handle; }
            set { this.settings.Handle = value; }
        }

        public uint? RemoteHandle
        {
            get;
            set;
        }

        public AmqpSession Session
        {
            get;
            private set;
        }

        public AmqpLinkSettings Settings
        {
            get { return this.settings; }
        }

        public bool IsReceiver
        {
            get
            {
                return this.settings.Role.Value;
            }
        }

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

        public virtual uint Available
        {
            get
            {
                return this.available;
            }
        }

        public uint MaxFrameSize
        {
            get;
            set;
        }

        protected Dictionary<ArraySegment<byte>, Delivery> UnsettledMap
        {
            get
            {
                return this.unsettledMap;
            }
        }

        protected object SyncRoot
        {
            get
            {
                return this.syncRoot;
            }
        }

        public void AttachTo(AmqpSession session)
        {
            Fx.Assert(this.Session == null, "The link is already attached to a session");
            this.MaxFrameSize = session.Connection.Settings.MaxFrameSize();
            this.Session = session;
            session.AttachLink(this);
        }

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
                AmqpTrace.Provider.AmqpLogError(this, nameof(ProcessFrame), exception.Message);
                this.SafeClose(exception);
            }
        }

        public void OnFlow(Flow flow)
        {
            this.OnReceiveFlow(flow);
        }

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

        // up-down: from application to link to session (to send a disposition)
        public void DisposeDelivery(Delivery delivery, bool settled, DeliveryState state)
        {
            this.DisposeDelivery(delivery, settled, state, false);
        }

        public void DisposeDelivery(Delivery delivery, bool settled, DeliveryState state, bool noFlush)
        {
            this.DoActionIfNotClosed(
                (thisPtr, paramDelivery, paramSettled, paramState, paramNoFlush) =>
                {
                    thisPtr.DisposeDeliveryInternal(paramDelivery, paramSettled, paramState, paramNoFlush);
                    return true;
                },
                delivery,
                settled,
                state,
                noFlush);
        }

        public bool DisposeDelivery(ArraySegment<byte> deliveryTag, bool settled, DeliveryState state, bool batchable)
        {
            Delivery delivery = null;
            if (this.unsettledMap.TryGetValue(deliveryTag, out delivery))
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
                    this.SendFlow(false, false, null);
                }
            }
        }

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

        public void NotifySessionCredit(int credit)
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
        public void OnDisposeDelivery(Delivery delivery)
        {
            if (delivery.Settled)
            {
                lock (this.syncRoot)
                {
                    this.unsettledMap.Remove(delivery.DeliveryTag);
                }

                this.OnDeliverySettled();
            }

            this.OnDisposeDeliveryInternal(delivery);

            // in case the receiver does not honor the settlement policy
            if (!this.IsReceiver && 
                this.settings.SettleType != SettleMode.SettleOnDispose && 
                !delivery.Settled &&
                !delivery.Transactional())
            {
                this.CompleteDelivery(delivery.DeliveryTag);
            }
        }

        public void CompleteDelivery(ArraySegment<byte> deliveryTag)
        {
            Delivery delivery = null;
            if (this.unsettledMap.TryGetValue(deliveryTag, out delivery))
            {
                this.DisposeDelivery(delivery, true, delivery.State);
            }
        }

        public void SendProperties(Fields properties)
        {
            this.SendFlow(false, false, properties);
        }

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
                if (this.IsReceiver)
                {
                    this.available = flow.Available ?? uint.MaxValue;
                    this.ApplyTempTotalLinkCredit();
                }
                else
                {
                    this.drain = flow.Drain ?? false;
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

            if (moreCredit > 0)
            {
                ArraySegment<byte> txnId = this.GetTxnIdFromFlow(flow);
                this.OnCreditAvailable(0, moreCredit, this.drain, txnId);
            }

            if (flow.Echo())
            {
                this.SendFlow(false, false, null);
            }

            return moreCredit;
        }

        // Returns true if this is a new delivery
        public abstract bool CreateDelivery(Transfer transfer, out Delivery delivery);

        internal virtual void OnIoEvent(IoEvent ioEvent)
        {
            EventHandler temp = this.PropertyReceived;
            if (temp != null)
            {
                AmqpTrace.Provider.AmqpIoEvent(this, (int)ioEvent, (long)this.linkCredit);

                Fields properties = new Fields();
                properties.Add(AmqpConstants.IoEvent, (int)ioEvent);
                temp(properties, EventArgs.Empty);
            }
        }

        bool IWorkDelegate<Delivery>.Invoke(Delivery delivery)
        {
            return this.DoActionIfNotClosed(
                (thisPtr, paramDelivery, p1, p2, p3) => thisPtr.SendDelivery(paramDelivery),
                delivery,
                0,
                0,
                0);
        }

        protected override bool OpenInternal()
        {
            AmqpObjectState state = this.SendAttach();
            if (this.IsReceiver)
            {
                lock (this.syncRoot)
                {
                    this.ApplyTempTotalLinkCredit();
                }
            }

            return state == AmqpObjectState.Opened;
        }

        protected override bool CloseInternal()
        {
            AmqpObjectState state = this.State;
            if (state == AmqpObjectState.OpenReceived ||
                state == AmqpObjectState.ClosePipe)
            {
                this.settings.Source = null;
                this.settings.Target = null;
                this.SendAttach();
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
            state = this.SendDetach();
            return state == AmqpObjectState.End;
        }

        protected override void AbortInternal()
        {
            Interlocked.Decrement(ref this.references);

            if (this.inflightDeliveries != null)
            {
                this.inflightDeliveries.Abort();
            }
        }

        protected void SendFlow(bool echo)
        {
            this.SendFlow(echo, false, null);
        }

        protected void ProcessTransfer(Transfer transfer, Frame rawFrame, Delivery delivery, bool newDelivery)
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
                            this.SendFlow(false, false, null);
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

                if (!delivery.Settled)
                {
                    lock (this.syncRoot)
                    {
                        this.unsettledMap.Add(delivery.DeliveryTag, delivery);
                    }
                }
            }

            this.OnProcessTransfer(delivery, transfer, rawFrame);
        }

        protected virtual void OnReceiveFlow(Flow flow)
        {
            this.ProcessFlow(flow);
        }

        protected abstract void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame rawFrame);

        protected abstract void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId);

        protected abstract void OnDisposeDeliveryInternal(Delivery delivery);

        protected virtual void OnReceiveStateOpenSent(Attach attach)
        {
            // no op by default
        }

        protected bool SendDelivery(Delivery delivery)
        {
            bool more = true;
            while (more)
            {
                bool firstTransfer = delivery.BytesTransfered == 0;
                Transfer transfer = new Transfer();
                transfer.Handle = this.LocalHandle;
                transfer.More = more;
                if (firstTransfer)
                {
                    transfer.DeliveryId = uint.MaxValue;    // reserve the space first
                    transfer.DeliveryTag = delivery.DeliveryTag;
                    transfer.MessageFormat = delivery.MessageFormat ?? AmqpConstants.AmqpMessageFormat;
                    transfer.Batchable = delivery.Batchable;
                    if (delivery.Settled)
                    {
                        transfer.Settled = true;
                    }

                    if (delivery.TxnId.Array != null)
                    {
                        transfer.State = new TransactionalState() { TxnId = delivery.TxnId };
                    }
                }

                uint maxFrameSize = this.MaxFrameSize == uint.MaxValue ? AmqpConstants.DefaultMaxFrameSize : this.MaxFrameSize;
                int overhead = Frame.HeaderSize + transfer.EncodeSize;
                if (overhead > maxFrameSize)
                {
                    throw new AmqpException(AmqpErrorCode.FrameSizeTooSmall, null);
                }

                int payloadSize = (int)maxFrameSize - overhead;
                ArraySegment<byte>[] payload = delivery.GetPayload(payloadSize, out more);
                transfer.More = more;

                if (!firstTransfer && payload == null)
                {
                    Fx.Assert(!more, "More flag is set but a null payload is returned.");
                    break;
                }

                if (!this.Session.TrySendTransfer(firstTransfer ? delivery : null, transfer, payload))
                {
                    more = true;
                    break;
                }

                payloadSize = 0;
                for (int i = 0; i < payload.Length; ++i)
                {
                    payloadSize += payload[i].Count;
                }

                delivery.CompletePayload(payloadSize);
            }

            if (!more && delivery.Settled)
            {
                delivery.State = AmqpConstants.AcceptedOutcome;
                this.OnDisposeDeliveryInternal(delivery);
            }

            return !more;
        }

        // call this method if the operation should not be performed after close
        protected bool DoActionIfNotClosed<T1, T2, T3, T4>(Func<AmqpLink, T1, T2, T3, T4, bool> action, T1 p1, T2 p2, T3 p3, T4 p4)
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
            delivery.PrepareForSend();
            delivery.Settled = this.settings.SettleType == SettleMode.SettleOnSend;
            if (!delivery.Settled)
            {
                lock (this.syncRoot)
                {
                    this.unsettledMap.Add(delivery.DeliveryTag, delivery);
                }
            }

            delivery.Link = this;
            this.inflightDeliveries.DoWork(delivery);
        }

        void DisposeDeliveryInternal(Delivery delivery, bool settled, DeliveryState state, bool noFlush)
        {
            AmqpTrace.Provider.AmqpDispose(this, delivery.DeliveryId.Value, settled, state);
            if (settled && !delivery.Settled)
            {
                lock (this.syncRoot)
                {
                    this.unsettledMap.Remove(delivery.DeliveryTag);
                }
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
                    if (this.needFlowCount >= this.settings.FlowThreshold)
                    {
                        sendFlow = true;
                        this.needFlowCount = 0;
                    }
                }

                if (sendFlow)
                {
                    this.SendFlow(false, false, null);
                }
            }
        }

        AmqpObjectState SendAttach()
        {
            StateTransition transition = this.TransitState("S:ATTACH", StateTransition.SendOpen);
            this.Session.SendCommand(this.settings);
            return transition.To;
        }

        AmqpObjectState SendDetach()
        {
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

                    this.OnReceiveStateOpenSent(attach);
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
                    this.Session.LinkFactory.BeginOpenLink(this, this.DefaultOpenTimeout, onProviderLinkOpened, this);
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
                delivery.Settled = transfer.Settled();
                delivery.Batchable = transfer.Batchable();
                delivery.MessageFormat = transfer.MessageFormat;
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
            if (attach.MaxMessageSize() != 0)
            {
                this.settings.MaxMessageSize = Math.Min(this.settings.MaxMessageSize(), attach.MaxMessageSize());
            }

            return null;
        }

        static void OnProviderLinkOpened(IAsyncResult result)
        {
            var thisPtr = (AmqpLink)result.AsyncState;
            AmqpTrace.Provider.AmqpLogOperationVerbose(thisPtr, TraceOperation.Execute, "OnProviderLinkOpened");
            Exception openException = null;

            // Capture the exception from provider first
            try
            {
                thisPtr.Session.LinkFactory.EndOpenLink(result);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpTrace.Provider.AmqpLogError(thisPtr, "EndOpenLink", exception.Message);
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
                AmqpTrace.Provider.AmqpLogError(thisPtr, "CompleteOpenLink", exception.Message);
                thisPtr.OnLinkOpenFailed(exception);
            }
        }

        void OnLinkOpenFailed(Exception exception)
        {
            if (this.State == AmqpObjectState.OpenReceived)
            {
                this.settings.Source = null;
                this.settings.Target = null;
                this.SendAttach();
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

        protected void SendFlow(bool echo, bool drain, Fields properties)
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

                if (!this.IsClosing())
                {
                    this.Session.SendFlow(flow);
                }
            }
            while (!forceSend && Interlocked.Decrement(ref this.sendingFlow) > 0);
        }
    }
}
