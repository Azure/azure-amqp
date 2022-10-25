// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Implements the AMQP 1.0 session.
    /// </summary>
    public class AmqpSession : AmqpObject
    {
        static readonly EventHandler onLinkClosed = OnLinkClosed;
        readonly AmqpConnection connection;
        readonly AmqpSessionSettings settings;
        readonly ILinkFactory linkFactory;
        Dictionary<AmqpLinkIdentifier, AmqpLink> links;
        HandleTable<AmqpLink> linksByLocalHandle;
        HandleTable<AmqpLink> linksByRemoteHandle;
        OutgoingSessionChannel outgoingChannel;
        IncomingSessionChannel incomingChannel;

        /// <summary>
        /// Initializes the session object.
        /// </summary>
        /// <param name="connection">The connection in which the session is created.</param>
        /// <param name="settings">The session settings.</param>
        /// <param name="linkFactory">The factory to create <see cref="AmqpLink"/> objects when an <see cref="Attach"/> frame is received.</param>
        public AmqpSession(AmqpConnection connection, AmqpSessionSettings settings, ILinkFactory linkFactory)
            : this("session", connection, settings, linkFactory)
        {
        }

        /// <summary>
        /// Initializes the session object.
        /// </summary>
        /// <param name="type">A prefix to the session name for debugging purposes.</param>
        /// <param name="connection">The connection in which the session is created.</param>
        /// <param name="settings">The session settings.</param>
        /// <param name="linkFactory">The factory to create <see cref="AmqpLink"/> objects when an <see cref="Attach"/> frame is received.</param>
        protected AmqpSession(string type, AmqpConnection connection, AmqpSessionSettings settings, ILinkFactory linkFactory)
            : base(type)
        {
            Fx.Assert(connection != null, "connection must not be null");
            Fx.Assert(settings != null, "settings must not be null");
            this.connection = connection;
            this.settings = settings;
            this.linkFactory = linkFactory;
            this.State = AmqpObjectState.Start;
            this.links = new Dictionary<AmqpLinkIdentifier, AmqpLink>();
            this.linksByLocalHandle = new HandleTable<AmqpLink>(settings.HandleMax ?? AmqpConstants.DefaultMaxLinkHandles - 1);
            this.linksByRemoteHandle = new HandleTable<AmqpLink>(settings.HandleMax ?? AmqpConstants.DefaultMaxLinkHandles - 1);
            this.outgoingChannel = new OutgoingSessionChannel(this);
            this.incomingChannel = new IncomingSessionChannel(this);
        }

        /// <summary>
        /// Gets the session settings.
        /// </summary>
        public AmqpSessionSettings Settings
        {
            get { return this.settings; }
        }

        /// <summary>
        /// Gets the owning connection.
        /// </summary>
        public AmqpConnection Connection
        {
            get { return this.connection; }
        }

        /// <summary>
        /// Gets the assigned local session channel number.
        /// </summary>
        public ushort LocalChannel
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the remote session channel.
        /// </summary>
        public ushort? RemoteChannel
        {
            get
            {
                return this.settings.RemoteChannel;
            }

            set
            {
                this.settings.RemoteChannel = value;
            }
        }

        /// <summary>
        /// Gets the link factory.
        /// </summary>
        public ILinkFactory LinkFactory
        {
            get
            {
                return this.linkFactory;
            }
        }

        internal override TimeSpan OperationTimeout => this.connection.OperationTimeout;

        /// <summary>
        /// Opens an <see cref="AmqpLink"/> to a node at the given address.
        /// </summary>
        /// <typeparam name="T">The type of link. Only <see cref="SendingAmqpLink"/> and <see cref="ReceivingAmqpLink"/> are supported.</typeparam>
        /// <param name="name">The link name.</param>
        /// <param name="address">The node address.</param>
        /// <returns>A task that returns a link on completion.</returns>
        public Task<T> OpenLinkAsync<T>(string name, string address) where T : AmqpLink
        {
            AmqpLinkSettings linkSettings = AmqpLinkSettings.Create<T>(name, address);
            return this.OpenLinkAsync<T>(linkSettings);
        }

        /// <summary>
        /// Open or resume a link using the <see cref="AmqpLinkSettings"/> provided.
        /// </summary>
        /// <typeparam name="T">The type of link.</typeparam>
        /// <param name="linkSettings">The link settings to be used for creating the new link.</param>
        /// <returns>A task that returns a link on completion.</returns>
        public async Task<T> OpenLinkAsync<T>(AmqpLinkSettings linkSettings) where T : AmqpLink
        {
            if (linkSettings == null)
            {
                throw new ArgumentNullException(nameof(linkSettings));
            }

            if (linkSettings.LinkName == null)
            {
                throw new ArgumentNullException(nameof(linkSettings.LinkName));
            }

            AmqpLink link = AmqpLink.Create(linkSettings);
            try
            {
                link.AttachTo(this);
                await link.OpenAsync().ConfigureAwait(false);
                return link as T;
            }
            catch
            {
                link.SafeClose();
                throw;
            }
        }

        /// <summary>
        /// Attaches a link to the session. The link is assigned a local handle on success.
        /// </summary>
        /// <param name="link">The link to attach.</param>
        public void AttachLink(AmqpLink link)
        {
            Fx.Assert(link.Session == this, "The link is not owned by this session.");
            AmqpLink linkToSteal;

            lock (this.ThisLock)
            {
                if (this.IsClosing())
                {
                    throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpIllegalOperationState, "attach", this.State));
                }

                if (this.links.TryGetValue(link.LinkIdentifier, out linkToSteal) && link.AllowLinkStealing(linkToSteal.Settings.GetAmqpLinkTerminusInfo()))
                {
                    // Even though link onclose handler already removes the link from the links collection,
                    // calling Close() is fire and forget, so we will not be waiting for the link onClose handler to trigger
                    // before trying to add the new link to the links collection down below in this method, therefore remove it now.
                    this.links.Remove(link.LinkIdentifier);
                }
                else
                {
                    linkToSteal = null;
                }

                if (this.Connection.LinkRecoveryEnabled)
                {
                    this.Connection.LinkTerminusManager.TryGetLinkTerminus(link.LinkIdentifier, out AmqpLinkTerminus linkTerminus);
                    if (linkTerminus == null)
                    {
                        linkTerminus = this.Connection.LinkTerminusManager.CreateLinkTerminus(link.LinkIdentifier, ((ILinkRecoveryRuntimeProvider)this.Connection.AmqpSettings.RuntimeProvider).UnsettledDeliveryStore);
                        if (!this.Connection.LinkTerminusManager.TryAddLinkTerminus(link.LinkIdentifier, linkTerminus))
                        {
                            // There was a race and some other link has already created a link terminus and attach.
                            // In this case, stop opening of this link and close it due to link stealing.
                            throw new AmqpException(AmqpErrorCode.Stolen, AmqpResources.GetString(AmqpResources.AmqpLinkStolen, link.LinkIdentifier));
                        }
                    }

                    if (!linkTerminus.TryAssociateLink(link, out linkToSteal))
                    {
                        throw new InvalidOperationException("The link terminus to attach this link to is either disposed or link stealing is not allowed.");
                    }
                }

                link.Closed += onLinkClosed;
                this.links.Add(link.LinkIdentifier, link);
                link.LocalHandle = this.linksByLocalHandle.Add(link);
            }

            if (linkToSteal != null)
            {
                bool sameConnection = link.Session.Connection.Settings.ContainerId.Equals(linkToSteal.Session.Connection.Settings.ContainerId, StringComparison.OrdinalIgnoreCase);
                bool sameSession = link.Session.RemoteChannel == linkToSteal.Session.RemoteChannel;

                // In case of half open links (remote has aborted a link, but local still has the link has fully open state),
                // and the remote link and the local link have the same connection/session handles,
                // sending a Detach to remote to close the exsting link due to link steal may cause the remote to mistakenly think
                // that the Detach is for this current link to be opened, instead of the exsting link to be closed for link stealing.
                // In that case, simply abort the existing link locally without sending any Detach to remote to avoid causing confusion.
                linkToSteal.OnLinkStolen(sameConnection && sameSession);
            }

            AmqpTrace.Provider.AmqpAttachLink(this.connection, this, link, link.LocalHandle.Value,
                link.RemoteHandle ?? 0u, link.Name, link.IsReceiver ? "receiver" : "sender", link.Settings.Source, link.Settings.Target);
        }

        /// <summary>
        /// Processes session and link frames.
        /// </summary>
        /// <param name="frame">The received frame.</param>
        public virtual void ProcessFrame(Frame frame)
        {
            Performative command = frame.Command;

            try
            {
                AmqpDebug.Log(this, false, command);

                if (command.DescriptorCode == Begin.Code)
                {
                    this.OnReceiveBegin((Begin)command);
                }
                else if (command.DescriptorCode == End.Code)
                {
                    this.OnReceiveEnd((End)command);
                }
                else if (command.DescriptorCode == Disposition.Code)
                {
                    this.OnReceiveDisposition((Disposition)command);
                }
                else if (command.DescriptorCode == Flow.Code)
                {
                    this.OnReceiveFlow((Flow)command);
                }
                else
                {
                    this.OnReceiveLinkFrame(frame);
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpTrace.Provider.AmqpLogError(this, "ProcessFrame", exception);
                this.SafeClose(exception);
            }
        }

        /// <summary>
        /// Sends the flow command after updating it with incoming and outgoing channel state.
        /// </summary>
        /// <param name="flow"></param>
        public void SendFlow(Flow flow)
        {
            lock (this.ThisLock)
            {
                if (!this.IsClosing())
                {
                    this.outgoingChannel.SendFlow(flow);
                }
            }
        }

        internal void SendCommand(Performative command)
        {
            this.SendCommand(command, null);
        }

        internal void SendCommand(Performative command, ByteBuffer payload)
        {
            AmqpDebug.Log(this, true, command);
            this.connection.SendCommand(command, this.LocalChannel, payload);
        }

        /// <summary>
        /// Attempts to send the transfer over the outgoing channel.
        /// </summary>
        /// <param name="delivery">The delivery owning the transfer.</param>
        /// <param name="transfer">The transfer command to send.</param>
        /// <param name="payload">The payload to carry in the transfer frame.</param>
        /// <returns>True if the transfer is sent; false if session window is 0.</returns>
        public bool TrySendTransfer(Delivery delivery, Transfer transfer, ByteBuffer payload)
        {
            // delivery MUST be null for continued transfer fragments
            return this.outgoingChannel.TrySendTransfer(delivery, transfer, payload);
        }

        /// <summary>
        /// Updates the state of the delivery and sends a disposition if required.
        /// </summary>
        /// <param name="link">The link where the delivery was transferred.</param>
        /// <param name="delivery">The delivery to update.</param>
        /// <param name="settled">Settle the delivery. See <see cref="Delivery.Settled"/> for more details.</param>
        /// <param name="state">The new state of the delivery.</param>
        /// <param name="noFlush">True to not send a disposition right away; false otherwise.</param>
        public void DisposeDelivery(AmqpLink link, Delivery delivery, bool settled, DeliveryState state, bool noFlush)
        {
            if (link.IsReceiver)
            {
                this.incomingChannel.DisposeDelivery(delivery, settled, state, noFlush);
            }
            else
            {
                this.outgoingChannel.DisposeDelivery(delivery, settled, state, noFlush);
            }
        }

        internal bool OnAcceptTransfer(Delivery delivery, Transfer transfer, bool newDelivery)
        {
            try
            {
                this.incomingChannel.OnAcceptTransfer(delivery, transfer, newDelivery);
                return true;
            }
            catch (AmqpException exception)
            {
                this.SafeClose(exception);
                return false;
            }
        }

        /// <summary>
        /// Sends a disposition for all pending delivery state changes in both incoming and outgoing channels.
        /// </summary>
        public void Flush()
        {
            this.outgoingChannel.Flush();
            this.incomingChannel.Flush();
        }

        internal void OnIoEvent(IoEvent ioEvent)
        {
            IEnumerator<AmqpLink> it = this.linksByLocalHandle.GetSafeEnumerator();
            while (it.MoveNext())
            {
                it.Current.OnIoEvent(ioEvent);
            }
        }

        /// <summary>
        /// Opens the session.
        /// </summary>
        /// <returns>True if the session is open; false if open is pending.</returns>
        protected override bool OpenInternal()
        {
            AmqpObjectState state = this.SendBegin();
            return state == AmqpObjectState.Opened;
        }

        /// <summary>
        /// Closes the session.
        /// </summary>
        /// <returns>True if the session is closed; false if close is pending.</returns>
        /// <remarks>All links in the session are also closed.</remarks>
        protected override bool CloseInternal()
        {
            AmqpObjectState state = this.State;
            if (state == AmqpObjectState.OpenReceived)
            {
                state = this.SendBegin();
            }

            this.CloseLinks(!this.LinkFrameAllowed());
            AmqpDebug.Dump(this);

            if (StateTransition.CanTransite(state, StateTransition.SendClose))
            {
                state = this.SendEnd();
            }
            else
            {
                state = this.State = AmqpObjectState.End;
            }

            return state == AmqpObjectState.End;
        }

        /// <summary>
        /// Aborts the session object. All links in the session are aborted.
        /// </summary>
        protected override void AbortInternal()
        {
            this.CloseLinks(true);
            AmqpDebug.Dump(this);
        }

        /// <summary>
        /// Sends a begin frame.
        /// </summary>
        /// <returns>The session state after sending the begin frame.</returns>
        protected AmqpObjectState SendBegin()
        {
            StateTransition transition = this.TransitState("S:BEGIN", StateTransition.SendOpen);
            this.SendCommand(this.settings);
            return transition.To;
        }

        /// <summary>
        /// Sends an end frame.
        /// </summary>
        /// <returns>The session state after sending the end frame.</returns>
        protected AmqpObjectState SendEnd()
        {
            StateTransition transition = this.TransitState("S:END", StateTransition.SendClose);

            End end = new End();
            Exception exception = this.TerminalException;
            if (exception != null)
            {
                end.Error = Error.FromException(exception);
            }

            this.SendCommand(end);
            return transition.To;
        }

        internal bool TryCreateRemoteLink(Attach attach, out AmqpLink link)
        {
            link = null;
            if (this.linkFactory == null)
            {
                return false;
            }

            AmqpLinkSettings linkSettings = AmqpLinkSettings.Create(attach);
            Exception error = null;

            try
            {
                link = this.LinkFactory.CreateLink(this, linkSettings);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpException amqpException = exception as AmqpException;
                if (amqpException != null &&
                    amqpException.Error != null &&
                    (amqpException.Error.Condition.Equals(AmqpErrorCode.ResourceLimitExceeded) ||
                     amqpException.Error.Condition.Equals(AmqpErrorCode.ResourceLocked)))
                {
                    // out of handle or link name exists
                    throw;
                }

                AmqpTrace.Provider.AmqpLogError(this, "CreateLink", exception);

                // detach requires a handle so the error link has to be attached first
                link = new ErrorLink(this, linkSettings);
                error = exception;
            }

            link.RemoteHandle = attach.Handle;
            this.linksByRemoteHandle.Add(attach.Handle.Value, link);

            if (error != null)
            {
                link.SafeClose(error);
                return false;
            }

            return true;
        }

        void CloseLinks(bool abort)
        {
            IEnumerable<AmqpLink> linksSnapshot = null;
            lock (this.ThisLock)
            {
                linksSnapshot = this.linksByLocalHandle.Values;
                if (abort)
                {
                    this.linksByLocalHandle.Clear();
                    this.linksByRemoteHandle.Clear();
                }
            }

            foreach (AmqpLink link in linksSnapshot)
            {
                if (abort)
                {
                    link.Abort();
                }
                else
                {
                    link.SafeClose(this.connection.TerminalException);
                }
            }
            this.incomingChannel.Close();
            this.outgoingChannel.Close();
        }

        bool LinkFrameAllowed()
        {
            return this.State == AmqpObjectState.OpenSent ||
                this.State == AmqpObjectState.Opened;
        }

        void SendFlow()
        {
            this.SendFlow(new Flow());
        }

        void OnReceiveBegin(Begin begin)
        {
            StateTransition stateTransition = this.TransitState("R:BEGIN", StateTransition.ReceiveOpen);

            this.incomingChannel.OnBegin(begin);
            this.NotifyOpening(begin);
            if (stateTransition.To == AmqpObjectState.OpenReceived)
            {
                this.outgoingChannel.OnBegin(begin);
                this.UpdateHandleTable(begin);
                this.Open();
            }
            else
            {
                Exception exception = null;
                Error error = this.Negotiate(begin);
                if (error != null)
                {
                    exception = new AmqpException(error);
                }

                this.CompleteOpen(false, exception);
                if (exception != null)
                {
                    this.SafeClose(exception);
                }
            }
        }

        void OnReceiveEnd(End end)
        {
            this.OnReceiveCloseCommand("R:END", end.Error);
        }

        void OnReceiveDisposition(Disposition disposition)
        {
            if (disposition.Role.Value)
            {
                this.outgoingChannel.OnReceiveDisposition(disposition);
            }
            else
            {
                this.incomingChannel.OnReceiveDisposition(disposition);
            }
        }

        /// <summary>
        /// Receives a flow from the session.
        /// </summary>
        /// <param name="flow">The received flow.</param>
        protected virtual void OnReceiveFlow(Flow flow)
        {
            this.outgoingChannel.OnFlow(flow);
            this.incomingChannel.OnFlow(flow);

            if (flow.Handle.HasValue)
            {
                AmqpLink link = null;
                if (!this.linksByRemoteHandle.TryGetObject(flow.Handle.Value, out link))
                {
                    if (this.Settings.IgnoreMissingLinks)
                    {
                        AmqpTrace.Provider.AmqpMissingHandle(this, "link", flow.Handle.Value);
                        return;
                    }

                    this.SafeClose(new AmqpException(AmqpErrorCode.UnattachedHandle, AmqpResources.GetString(AmqpResources.AmqpHandleNotFound, flow.Handle.Value, this)));
                    return;
                }

                link.OnFlow(flow);
            }
            else if (flow.Echo())
            {
                this.SendFlow();
            }
        }

        void OnReceiveLinkFrame(Frame frame)
        {
            AmqpLink link = null;
            AmqpLink stolenLink = null;
            Performative command = frame.Command;
            if (command.DescriptorCode == Attach.Code)
            {
                Attach attach = (Attach)command;
                var linkIdentifier = new AmqpLinkIdentifier(attach.LinkName, !attach.IsReceiver(), this.Connection.Settings.ContainerId); // local Role will be opposite of the Role sent from remote for the same link.

                lock (this.ThisLock)
                {
                    this.links.TryGetValue(linkIdentifier, out link);
                    if (link != null && link.State >= AmqpObjectState.OpenReceived)
                    {
                        // If the link state is past OpenReceived, it means that the link has already received an Attach frame from remote, regardless if the link open was initiated by local or remote.
                        // A single link life cycle should not receive Attach more than once, therefore if the existing link has already received an Attach, this current Attach must be intended for another link.
                        // In that case, remove the existing link due to link stealing and create a new link based on the Attach frame received.
                        stolenLink = link;
                        this.links.Remove(linkIdentifier);
                    }
                }

                if (stolenLink != null)
                {
                    // Abort the local existing link, which does not send a detach back to remote, who initiated opening a new link and is not expecting the new link to be closed.
                    stolenLink.OnLinkStolen(true);
                    link = null;
                }

                if (link == null || link.State >= AmqpObjectState.OpenReceived)
                {
                    // If the link state is past OpenReceived, it means that the link has already received an Attach frame from remote, regardless if the link open was initiated by local or remote.
                    // A single link should receive Attach only once, therefore if the existing link has already received an Attach, then the current Attach must be intended for opening another link.
                    // In that case, we can try to open a new link and potentially do link stealing.
                    if (!this.TryCreateRemoteLink(attach, out link))
                    {
                        return;
                    }
                }
                else
                {
                    // This scenario indicates that the existing link has already been created locally but no Attach has been received yet.
                    // This could only mean that the link open was initiated from local, so this Attach frame received is the reply from remote in response to the initial Attach sent by local.
                    // Therefore, we do not need to open or close anything from local side because we just need to complete the open process locally.
                    lock (this.ThisLock)
                    {
                        link.RemoteHandle = attach.Handle;
                        this.linksByRemoteHandle.Add(attach.Handle.Value, link);
                    }
                }
            }
            else
            {
                LinkPerformative linkBody = (LinkPerformative)command;
                if (!this.linksByRemoteHandle.TryGetObject(linkBody.Handle.Value, out link))
                {
                    if (this.Settings.IgnoreMissingLinks)
                    {
                        AmqpTrace.Provider.AmqpMissingHandle(this, "link", linkBody.Handle.Value);
                        return;
                    }

                    if (linkBody.DescriptorCode != Detach.Code)
                    {
                        this.SafeClose(new AmqpException(AmqpErrorCode.UnattachedHandle, AmqpResources.GetString(AmqpResources.AmqpHandleNotFound, linkBody.Handle.Value, this)));
                    }

                    return;
                }
            }

            link.ProcessFrame(frame);
        }

        void UpdateHandleTable(Begin begin)
        {
            this.settings.HandleMax = Math.Min(this.settings.HandleMax(), begin.HandleMax());
            this.linksByLocalHandle.SetMaxHandle(this.settings.HandleMax.Value);
            this.linksByRemoteHandle.SetMaxHandle(this.settings.HandleMax.Value);
        }

        Error Negotiate(Begin begin)
        {
            this.outgoingChannel.OnBegin(begin);
            this.UpdateHandleTable(begin);
            return null;
        }

        void NotifyCreditAvailable(int credit)
        {
            IEnumerable<AmqpLink> links = null;
            lock (this.ThisLock)
            {
                links = this.linksByLocalHandle.Values;
            }

            // Make it more fair to all links
            foreach (AmqpLink link in links)
            {
                if (!link.IsReceiver)
                {
                    link.NotifySessionCredit(credit);
                }
            }
        }

        static void OnLinkClosed(object sender, EventArgs e)
        {
            AmqpLink link = (AmqpLink)sender;
            AmqpSession thisPtr = link.Session;
            lock (thisPtr.ThisLock)
            {
                link.Closed -= onLinkClosed;
                if (thisPtr.links.TryGetValue(link.LinkIdentifier, out AmqpLink existingLink) && link == existingLink)
                {
                    thisPtr.links.Remove(link.LinkIdentifier);
                }

                if (link.LocalHandle.HasValue)
                {
                    thisPtr.linksByLocalHandle.Remove(link.LocalHandle.Value);
                }

                if (link.RemoteHandle.HasValue)
                {
                    thisPtr.linksByRemoteHandle.Remove(link.RemoteHandle.Value);
                }
            }

            thisPtr.incomingChannel.OnLinkClosed(link);
            thisPtr.outgoingChannel.OnLinkClosed(link);
            AmqpTrace.Provider.AmqpRemoveLink(thisPtr, link, link.LocalHandle ?? 0u, link.RemoteHandle ?? 0u, link.Name);
        }

        abstract class SessionChannel
        {
            readonly AmqpSession session;
            readonly object syncRoot;
            Timer dispositionTimer;
            SequenceNumber nextDeliveryId;
            int needDispositionCount;
            bool sendingDisposition;
            bool timerScheduled;
            Delivery firstUnsettled;
            Delivery lastUnsettled;

            public SessionChannel(AmqpSession session)
            {
                this.session = session;
                this.nextDeliveryId = session.settings.InitialDeliveryId;
                this.syncRoot = new object();
            }

            protected AmqpSession Session
            {
                get { return this.session; }
            }

            protected bool IsReceiver
            {
                get;
                set;
            }

            protected object SyncRoot
            {
                get { return this.syncRoot; }
            }

            public void OnLinkClosed(AmqpLink link)
            {
                int settledCount = 0;
                lock (this.syncRoot)
                {
                    Delivery current = this.firstUnsettled;
                    while (current != null)
                    {
                        Delivery delivery = current;
                        current = current.Next;

                        if (delivery.Link == link)
                        {
                            Delivery.Remove(ref this.firstUnsettled, ref this.lastUnsettled, delivery);
                            settledCount++;
                        }
                    }
                }

                if (settledCount > 0)
                {
                    this.OnWindowMoved(settledCount);
                }
            }

            public void OnReceiveDisposition(Disposition disposition)
            {
                SequenceNumber first = disposition.First.Value;
                SequenceNumber last = disposition.Last ?? first;
                if (last < first)
                {
                    // Should be a protocol error
                    return;
                }

                List<Delivery> disposedDeliveries = new List<Delivery>();
                int settledCount = 0;
                lock (this.syncRoot)
                {
                    if (first >= this.nextDeliveryId)
                    {
                        return;
                    }

                    if (last > this.nextDeliveryId)
                    {
                        last = this.nextDeliveryId;
                    }

                    bool settled = disposition.Settled();
                    Delivery current = this.firstUnsettled;
                    while (current != null)
                    {
                        SequenceNumber sn = current.DeliveryId.Value;
                        if (sn < first)
                        {
                            current = current.Next;
                        }
                        else if (sn > last)
                        {
                            break;
                        }
                        else
                        {
                            Delivery delivery = current;
                            current = current.Next;

                            delivery.Settled = settled;
                            delivery.State = disposition.State;
                            if (settled)
                            {
                                ++settledCount;
                                Delivery.Remove(ref this.firstUnsettled, ref this.lastUnsettled, delivery);
                            }

                            disposedDeliveries.Add(delivery);
                        }
                    }
                }

                if (disposedDeliveries.Count > 0)
                {
                    foreach (Delivery delivery in disposedDeliveries)
                    {
                        delivery.Link.OnDisposeDelivery(delivery);
                    }

                    if (settledCount > 0)
                    {
                        this.OnWindowMoved(settledCount);
                    }
                }
            }

            public void Flush()
            {
                // Make sure everything is sent out; otherwise a flow/disposition
                // maybe sent after the end frame
                SpinWait.SpinUntil(() => this.TrySendDisposition());
            }

            public void DisposeDelivery(Delivery delivery, bool settled, DeliveryState state, bool noFlush)
            {
                if (delivery.Settled)
                {
                    this.OnWindowMoved(1);
                    return;
                }

                bool scheduleTimer = false;
                Delivery toDispose = null;

                lock (this.syncRoot)
                {
                    delivery.StateChanged = true;
                    delivery.Settled = settled;
                    delivery.State = state;

                    if (!delivery.Batchable)
                    {
                        delivery.StateChanged = false;
                        toDispose = delivery;
                        if (delivery.Settled)
                        {
                            Delivery.Remove(ref this.firstUnsettled, ref this.lastUnsettled, delivery);
                        }
                    }
                    else if (this.sendingDisposition || noFlush)
                    {
                        return;
                    }
                    else if (this.session.settings.DispositionInterval == TimeSpan.Zero ||
                        ++this.needDispositionCount >= this.session.settings.DispositionThreshold)
                    {
                        this.sendingDisposition = true;
                        this.needDispositionCount = 0;
                    }
                    else if (!this.timerScheduled)
                    {
                        this.timerScheduled = true;
                        scheduleTimer = true;
                    }
                }

                if (toDispose != null)
                {
                    this.SendDisposition(new DispositionInfo(toDispose));
                    if (delivery.Settled)
                    {
                        this.OnWindowMoved(1);
                    }
                }
                else if (scheduleTimer)
                {
                    if (this.dispositionTimer == null)
                    {
                        this.dispositionTimer = new Timer(
                            s => DispositionTimerCallback(s),
                            this,
                            this.session.settings.DispositionInterval,
                            Timeout.InfiniteTimeSpan);
                    }
                    else
                    {
                        this.dispositionTimer.Change(this.session.settings.DispositionInterval, Timeout.InfiniteTimeSpan);
                    }
                }
                else
                {
                    this.SendDisposition();
                }
            }

            protected void OnSendDelivery(Delivery delivery)
            {
                // Caller should hold the lock
                delivery.DeliveryId = this.nextDeliveryId;
                this.nextDeliveryId.Increment();
                if (!delivery.Settled)
                {
                    Delivery.Add(ref this.firstUnsettled, ref this.lastUnsettled, delivery);
                }
            }

            protected void OnReceiveDelivery(Delivery delivery)
            {
                // this is always the next expected delivery id
                this.nextDeliveryId = delivery.DeliveryId + 1;
                if (!delivery.Settled)
                {
                    Delivery.Add(ref this.firstUnsettled, ref this.lastUnsettled, delivery);
                }
            }

            protected void OnReceiveFirstTransfer(Transfer transfer)
            {
                Fx.Assert(transfer.DeliveryId.HasValue, "The first transfer must have a delivery id.");
                this.nextDeliveryId = transfer.DeliveryId.Value;
            }

            protected abstract void OnWindowMoved(int count);

            static bool CanBatch(Outcome outcome1, Outcome outcome2)
            {
                // Only batch accepted and released outcomes
                return outcome1 != null &&
                    outcome2 != null &&
                    outcome1.DescriptorCode == outcome2.DescriptorCode &&
                    (outcome1.DescriptorCode == Accepted.Code || outcome1.DescriptorCode == Released.Code);
            }

            static void DispositionTimerCallback(object state)
            {
                SessionChannel thisPtr = (SessionChannel)state;
                if (thisPtr.session.State != AmqpObjectState.Opened)
                {
                    return;
                }

                AmqpTrace.Provider.AmqpLogOperationVerbose(thisPtr, TraceOperation.Execute, nameof(DispositionTimerCallback));

                lock (thisPtr.syncRoot)
                {
                    thisPtr.timerScheduled = false;
                    if (thisPtr.sendingDisposition)
                    {
                        return;
                    }

                    thisPtr.sendingDisposition = true;
                }

                try
                {
                    thisPtr.SendDisposition();
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    thisPtr.session.SafeClose(exception);
                }
            }

            bool TrySendDisposition()
            {
                lock (this.syncRoot)
                {
                    if (this.sendingDisposition)
                    {
                        return false;
                    }

                    this.sendingDisposition = true;
                    this.needDispositionCount = 0;
                }

                this.SendDisposition();
                return true;
            }

            void SendDisposition()
            {
                List<DispositionInfo> disposedDeliveries = new List<DispositionInfo>();
                int settledCount = 0;

                lock (this.syncRoot)
                {
                    Delivery current = this.firstUnsettled;
                    Delivery firstChanged = null;
                    uint? lastId = null;
                    while (current != null)
                    {
                        if (current.StateChanged)
                        {
                            if (firstChanged == null)
                            {
                                firstChanged = current;
                            }
                            else
                            {
                                if (current.Settled == firstChanged.Settled &&
                                    CanBatch(current.State as Outcome, firstChanged.State as Outcome))
                                {
                                    lastId = current.DeliveryId.Value;
                                }
                                else
                                {
                                    disposedDeliveries.Add(new DispositionInfo(firstChanged, lastId));
                                    firstChanged = current;
                                    lastId = null;
                                }
                            }

                            // Move next and remove if settled
                            if (current.Settled)
                            {
                                Delivery temp = current;
                                current = current.Next;
                                ++settledCount;
                                Delivery.Remove(ref this.firstUnsettled, ref this.lastUnsettled, temp);
                            }
                            else
                            {
                                current.StateChanged = false;
                                current = current.Next;
                            }
                        }
                        else
                        {
                            if (firstChanged != null)
                            {
                                disposedDeliveries.Add(new DispositionInfo(firstChanged, lastId));
                                firstChanged = null;
                                lastId = null;
                            }

                            current = current.Next;
                        }
                    }

                    if (firstChanged != null)
                    {
                        disposedDeliveries.Add(new DispositionInfo(firstChanged, lastId));
                    }

                    this.sendingDisposition = false;
                }

                if (disposedDeliveries.Count > 0)
                {
                    foreach (var info in disposedDeliveries)
                    {
                        this.SendDisposition(info);
                    }
                }

                if (settledCount > 0)
                {
                    this.OnWindowMoved(settledCount);
                }
            }

            void SendDisposition(DispositionInfo info)
            {
                Disposition disposition = new Disposition();
                disposition.First = info.First.DeliveryId.Value;
                disposition.Last = info.Last;
                disposition.Settled = info.First.Settled;
                disposition.State = info.First.State;
                disposition.Role = this.IsReceiver;

                lock (this.session.ThisLock)
                {
                    if (this.session.State < AmqpObjectState.CloseSent)
                    {
                        this.session.SendCommand(disposition);
                    }
                }
            }

            public void Close()
            {
                if (dispositionTimer != null)
                {
                    dispositionTimer.Dispose();
                }
            }

            readonly struct DispositionInfo
            {
                public DispositionInfo(Delivery first, uint? last = null)
                {
                    First = first;
                    Last = last;
                }

                public Delivery First { get; }

                public uint? Last { get; }
            }
        }

        sealed class OutgoingSessionChannel : SessionChannel
        {
            SequenceNumber nextOutgoingId;
            uint outgoingWindow;

            public OutgoingSessionChannel(AmqpSession session)
                : base(session)
            {
                this.nextOutgoingId = session.settings.NextOutgoingId.Value;
                this.outgoingWindow = session.settings.OutgoingWindow.Value;
                this.IsReceiver = false;
            }

            public bool TrySendTransfer(Delivery delivery, Transfer transfer, ByteBuffer payload)
            {
                lock (this.SyncRoot)
                {
                    if (this.outgoingWindow == 0)
                    {
                        AmqpTrace.Provider.AmqpSessionWindowClosed(this.Session, (int)this.nextOutgoingId.Value);
                        return false;
                    }

                    this.nextOutgoingId.Increment();
                    if (this.outgoingWindow < uint.MaxValue)
                    {
                        this.outgoingWindow--;
                    }

                    if (delivery != null)
                    {
                        this.OnSendDelivery(delivery);
                        transfer.DeliveryId = delivery.DeliveryId.Value;
                    }

                    this.Session.SendCommand(transfer, payload);
                }

                return true;
            }

            public void SendFlow(Flow flow)
            {
                lock (this.SyncRoot)
                {
                    // Outgoing state needs to be sync'ed with transfers
                    this.AddFlowState(flow, false);
                    this.Session.incomingChannel.AddFlowState(flow, true);
                    this.Session.SendCommand(flow, null);
                }
            }

            public void OnBegin(Begin begin)
            {
                lock (this.SyncRoot)
                {
                    // Can happen in pipeline mode
                    uint alreadySent = this.Session.settings.OutgoingWindow.Value - this.outgoingWindow;
                    if (alreadySent > begin.IncomingWindow.Value)
                    {
                        this.outgoingWindow = 0;
                    }
                    else
                    {
                        this.outgoingWindow = begin.IncomingWindow.Value - alreadySent;
                    }

                    this.Session.settings.OutgoingWindow = this.outgoingWindow;
                }
            }

            public void OnFlow(Flow flow)
            {
                uint remoteWindow = 0;
                lock (this.SyncRoot)
                {
                    if (flow.IncomingWindow.Value < uint.MaxValue)
                    {
                        uint flowNextIncomingId = flow.NextIncomingId ?? 0;
                        this.outgoingWindow = flowNextIncomingId + flow.IncomingWindow.Value - this.nextOutgoingId.Value;
                        remoteWindow = this.outgoingWindow;
                    }
                    else
                    {
                        // only notify links when the current window control is on
                        remoteWindow = this.outgoingWindow == uint.MaxValue ? 0 : uint.MaxValue;
                        this.outgoingWindow = uint.MaxValue;
                    }
                }

                if (remoteWindow > 0)
                {
                    this.Session.NotifyCreditAvailable(remoteWindow > int.MaxValue ? int.MaxValue : (int)remoteWindow);
                }
            }

            public void AddFlowState(Flow flow, bool reset)
            {
                lock (this.SyncRoot)
                {
                    flow.OutgoingWindow = this.outgoingWindow;
                    flow.NextOutgoingId = this.nextOutgoingId.Value;
                }
            }

            public override string ToString()
            {
                return this.Session.ToString() + "-out";
            }

            protected override void OnWindowMoved(int count)
            {
            }
        }

        sealed class IncomingSessionChannel : SessionChannel
        {
            SequenceNumber nextIncomingId;  // implicit next transfer id
            uint incomingWindow;
            uint flowThreshold;
            uint needFlowCount;
            bool transferEverReceived;

            public IncomingSessionChannel(AmqpSession session)
                : base(session)
            {
                this.incomingWindow = session.settings.IncomingWindow();
                this.flowThreshold = this.incomingWindow * 2 / 3;
                this.IsReceiver = true;
            }

            public void OnAcceptTransfer(Delivery delivery, Transfer transfer, bool newDelivery)
            {
                if (!this.transferEverReceived)
                {
                    this.OnReceiveFirstTransfer(transfer);
                    this.transferEverReceived = true;
                }

                bool canAccept = false;
                lock (this.SyncRoot)
                {
                    if (this.incomingWindow > 0)
                    {
                        canAccept = true;
                        if (newDelivery)
                        {
                            this.OnReceiveDelivery(delivery);
                        }

                        this.nextIncomingId.Increment();
                        if (this.incomingWindow < uint.MaxValue)
                        {
                            this.incomingWindow--;
                        }
                    }
                }

                if (!canAccept)
                {
                    AmqpTrace.Provider.AmqpSessionWindowClosed(this.Session, (int)this.nextIncomingId.Value);
                    throw new AmqpException(AmqpErrorCode.WindowViolation, null);
                }

                if (!newDelivery)
                {
                    // continued transfer for existing delivery: move window forward
                    this.OnWindowMoved(1);
                }
            }

            public void OnBegin(Begin begin)
            {
                lock (this.SyncRoot)
                {
                    this.nextIncomingId = begin.NextOutgoingId.Value;
                }
            }

            public void OnFlow(Flow flow)
            {
            }

            public void AddFlowState(Flow flow, bool reset)
            {
                lock (this.SyncRoot)
                {
                    flow.NextIncomingId = this.nextIncomingId.Value;
                    flow.IncomingWindow = this.incomingWindow;

                    if (reset)
                    {
                        this.needFlowCount = 0;
                    }
                }
            }

            public override string ToString()
            {
                return this.Session.ToString() + "-in";
            }

            protected override void OnWindowMoved(int count)
            {
                bool sendFlow = false;
                lock (this.SyncRoot)
                {
                    if (this.incomingWindow < uint.MaxValue)
                    {
                        this.incomingWindow += (uint)count;
                        this.needFlowCount += (uint)count;
                        if (this.needFlowCount >= this.flowThreshold)
                        {
                            this.needFlowCount = 0;
                            sendFlow = true;
                        }
                    }
                }

                if (sendFlow)
                {
                    this.Session.SendFlow();
                }
            }
        }

        sealed class ErrorLink : AmqpLink
        {
            public ErrorLink(AmqpSession session, AmqpLinkSettings settings)
                : base("error-link", session, settings)
            {
                settings.Properties = null;
            }

            protected override bool CloseInternal()
            {
                this.State = AmqpObjectState.OpenReceived;
                return base.CloseInternal();
            }

            protected override bool CreateDelivery(Transfer transfer, out Delivery delivery)
            {
                throw new NotImplementedException();
            }

            protected override void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId)
            {
                throw new NotImplementedException();
            }

            protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame rawFrame)
            {
                throw new NotImplementedException();
            }

            protected override void OnDisposeDeliveryInternal(Delivery delivery)
            {
                throw new NotImplementedException();
            }

            protected override void ProcessUnsettledDeliveries(Attach remoteAttach)
            {
                throw new NotImplementedException();
            }
        }
    }
}
