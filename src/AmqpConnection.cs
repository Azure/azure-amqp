// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;
    using CloseCommand = Microsoft.Azure.Amqp.Framing.Close;
    using OpenCommand = Microsoft.Azure.Amqp.Framing.Open;

    /// <summary>
    /// Implements the AMQP 1.0 connection.
    /// </summary>
    public class AmqpConnection : AmqpConnectionBase, ISessionFactory
    {
        static readonly EventHandler onSessionClosed = OnSessionClosed;
        readonly bool isInitiator;
        readonly ProtocolHeader initialHeader;
        readonly AmqpSettings amqpSettings;
        readonly HandleTable<AmqpSession> sessionsByLocalHandle;
        readonly HandleTable<AmqpSession> sessionsByRemoteHandle;
        HeartBeat heartBeat;
        Dictionary<Type, object> extensions;
        ConcurrentDictionary<AmqpLinkSettings, AmqpLink> recoverableLinkEndpoints;

        /// <summary>
        /// The default factory instance to create connections.
        /// </summary>
        // create a new instance as the caller many change the settings of the returned value.
        public static AmqpConnectionFactory Factory => new AmqpConnectionFactory();

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        public AmqpConnection(TransportBase transport, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this(transport, amqpSettings.GetDefaultHeader(), true, amqpSettings, connectionSettings)
        {
        }

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header for version negotiation.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        public AmqpConnection(TransportBase transport, ProtocolHeader protocolHeader, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this(transport, protocolHeader, true, amqpSettings, connectionSettings)
        {
        }

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header for version negotiation.</param>
        /// <param name="isInitiator">True if the connection is the initiator; false otherwise.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        public AmqpConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this((isInitiator ? "out" : "in") + "-connection", transport, protocolHeader, isInitiator, amqpSettings, connectionSettings)
        {
        }

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="type">A prefix for the connection name for debugging purposes.</param>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header for version negotiation.</param>
        /// <param name="isInitiator">True if the connection is the initiator; false otherwise.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        protected AmqpConnection(string type, TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            base(type, transport, connectionSettings, isInitiator)
        {
            if (amqpSettings == null)
            {
                throw new ArgumentNullException(nameof(amqpSettings));
            }

            this.initialHeader = protocolHeader;
            this.isInitiator = isInitiator;
            this.amqpSettings = amqpSettings;
            this.sessionsByLocalHandle = new HandleTable<AmqpSession>(this.Settings.ChannelMax ?? AmqpConstants.DefaultMaxConcurrentChannels - 1);
            this.sessionsByRemoteHandle = new HandleTable<AmqpSession>(this.Settings.ChannelMax ?? AmqpConstants.DefaultMaxConcurrentChannels - 1);
            this.SessionFactory = this;
            this.heartBeat = HeartBeat.None;
            if (connectionSettings.EnableLinkRecovery)
            {
                this.recoverableLinkEndpoints = new ConcurrentDictionary<AmqpLinkSettings, AmqpLink>();
            }
        }

        /// <summary>
        /// Gets the protocol settings of the connection.
        /// </summary>
        public AmqpSettings AmqpSettings
        {
            get { return this.amqpSettings; }
        }

        /// <summary>
        /// Gets or sets a session factory.
        /// </summary>
        public ISessionFactory SessionFactory
        {
            get;
            set;
        }

        /// <summary>
        /// Gets a key-value collection for storing objects with the connection.
        /// </summary>
        /// <remarks><see cref="Extensions.AddExtension(AmqpConnection, object)"/> and
        /// <see cref="Extensions.TryGetExtension{T}(AmqpConnection, out T)"/> should be used
        /// to add or find objects from the collection.</remarks>
        public IDictionary<Type, object> Extensions
        {
            get
            {
                if (this.extensions == null)
                {
                    this.extensions = new Dictionary<Type, object>();
                }

                return this.extensions;
            }
        }

        /// <summary>
        /// Gets a boolean value that indicates if the connection is the initiator.
        /// </summary>
        public bool IsInitiator
        {
            get { return this.isInitiator; }
        }

        /// <summary>
        /// Gets a snapshot of the sessions in the connection.
        /// </summary>
        /// <remarks>The property creates a new list for the snapshot. Avoid getting this
        /// property very frequently where there are a large number of sessions.</remarks>
        public IEnumerable<AmqpSession> Sessions
        {
            get
            {
                lock (this.ThisLock)
                {
                    return this.sessionsByLocalHandle.Values;
                }
            }
        }

        /// <summary>
        /// The link termini used by link recovery. The combination of link name+role must be unique under the connection scope to enable link recovery.
        /// Should only be initialized if link recovery is enabled on the connection settings.
        /// </summary>
        internal ConcurrentDictionary<AmqpLinkSettings, AmqpLink> RecoverableLinkEndpoints
        {
            get
            {
                return this.recoverableLinkEndpoints;
            }
        }

        /// <summary>
        /// Creates a <see cref="AmqpSession"/> and adds it to the session collection.
        /// </summary>
        /// <param name="sessionSettings">The session settings.</param>
        /// <returns>A session object.</returns>
        public AmqpSession CreateSession(AmqpSessionSettings sessionSettings)
        {
            AmqpSession session = this.SessionFactory.CreateSession(this, sessionSettings);
            this.AddSession(session, null);
            return session;
        }

        /// <summary>
        /// Opens a <see cref="AmqpSession"/> in the connection with default session settings.
        /// </summary>
        /// <returns>A task that returns an AmqpSession on completion.</returns>
        public Task<AmqpSession> OpenSessionAsync()
        {
            return this.OpenSessionAsync(new AmqpSessionSettings());
        }

        /// <summary>
        /// Opens a <see cref="AmqpSession"/> in the connection.
        /// </summary>
        /// <param name="sessionSettings">The session settings.</param>
        /// <returns>A task that returns an AmqpSession on completion.</returns>
        public async Task<AmqpSession> OpenSessionAsync(AmqpSessionSettings sessionSettings)
        {
            AmqpSession session = this.CreateSession(sessionSettings);
            try
            {
                await session.OpenAsync().ConfigureAwait(false);
                return session;
            }
            catch
            {
                session.SafeClose();
                throw;
            }
        }

        /// <summary>
        /// For internal implementation only.
        /// </summary>
        public void SendCommand(Performative command, ushort channel, ByteBuffer payload)
        {
            int frameSize;
            if (payload == null)
            {
                // The frame buffer is disposed when the write completes
                ByteBuffer buffer = Frame.EncodeCommand(FrameType.Amqp, channel, command, 0);
                frameSize = buffer.Length;
                this.SendBuffer(buffer);
            }
            else
            {
                // The frame buffer is disposed when the write completes
                ByteBuffer cmdBuffer = Frame.EncodeCommand(FrameType.Amqp, channel, command, payload.Length);
                frameSize = cmdBuffer.Length + payload.Length;
                this.SendBuffer(cmdBuffer, payload);
            }

            AmqpTrace.OnFrame(this.Identifier.Value, FrameType.Amqp, channel, command, true, frameSize);
            this.heartBeat.OnSend();
            if (this.UsageMeter != null)
            {
                this.UsageMeter.OnWrite(this, command == null ? 0 : command.DescriptorCode, frameSize);
            }
        }

        /// <summary>
        /// Opens the connection. Override this method if the derived class has extra or different
        /// operations.
        /// </summary>
        /// <returns>True if the connection enters open state; false if open is pending.</returns>
        /// <remarks>A connection is fully open when an <see cref="Open"/> frame is received from the
        /// remote peer.</remarks>
        protected override bool OpenInternal()
        {
            AmqpTrace.Provider.AmqpOpenConnection(this, this);
            if (this.isInitiator)
            {
                this.SendProtocolHeader(this.initialHeader);
                this.SendOpen();
                this.AsyncIO.Open();
            }
            else if (this.initialHeader != null)
            {
                this.OnProtocolHeader(this.initialHeader);
                this.AsyncIO.Open();
            }
            else
            {
                this.AsyncIO.Open();
            }

            return false;
        }

        /// <summary>
        /// Closes the connection. Override this method if the derived class has extra or different
        /// operations.
        /// </summary>
        /// <returns>True if the connection enters closed state; false if close is pending.</returns>
        /// <remarks>A connection is fully closed when an <see cref="Close"/> frame is received from the
        /// remote peer.</remarks>
        protected override bool CloseInternal()
        {
            AmqpTrace.Provider.AmqpCloseConnection(this, this, false);
            this.heartBeat.Stop();
            this.CloseSessions(!this.SessionFrameAllowed());

            if (this.State == AmqpObjectState.OpenReceived)
            {
                this.SendOpen();
            }

            try
            {
                this.SendClose();
            }
            catch (AmqpException)
            {
                this.State = AmqpObjectState.End;
            }

            bool completed = this.State == AmqpObjectState.End;
            if (completed)
            {
                this.AsyncIO.SafeClose();
            }

            return completed;
        }

        /// <summary>
        /// Aborts the connection. Override this method if the derived class has extra or different
        /// operations.
        /// </summary>
        /// <remarks>Abort shuts down the transport immediately without doing a close handshake
        /// with the remote peer.</remarks>
        protected override void AbortInternal()
        {
            AmqpTrace.Provider.AmqpCloseConnection(this, this, true);
            this.heartBeat.Stop();
            this.CloseSessions(true);
            this.AsyncIO.Abort();
        }

        /// <summary>
        /// Handles the received protocol header.
        /// </summary>
        /// <param name="header">The received protocol header.</param>
        protected override void OnProtocolHeader(ProtocolHeader header)
        {
            AmqpTrace.OnProtocolHeader(header, false);
            this.heartBeat.OnReceive();
            if (this.UsageMeter != null)
            {
                this.UsageMeter.OnRead(this, 0, header.EncodeSize);
            }

            this.TransitState("R:HDR", StateTransition.ReceiveHeader);
            Exception exception = null;

            if (this.isInitiator)
            {
                if (!this.initialHeader.Equals(header))
                {
                    exception = new AmqpException(AmqpErrorCode.NotImplemented, AmqpResources.GetString(AmqpResources.AmqpProtocolVersionNotSupported, this.initialHeader.ToString(), header.ToString()));
                }
            }
            else
            {
                ProtocolHeader supportedHeader = this.amqpSettings.GetSupportedHeader(header);
                this.SendProtocolHeader(supportedHeader);
                if (!supportedHeader.Equals(header))
                {
                    exception = new AmqpException(AmqpErrorCode.NotImplemented, AmqpResources.GetString(AmqpResources.AmqpProtocolVersionNotSupported, this.initialHeader.ToString(), header.ToString()));
                }
            }

            if (exception != null)
            {
                this.CompleteOpen(false, exception);
            }
        }

        /// <summary>
        /// Handles the received frame buffer.
        /// </summary>
        /// <param name="buffer">The received frame buffer.</param>
        protected override void OnFrameBuffer(ByteBuffer buffer)
        {
            if (this.State == AmqpObjectState.End)
            {
                buffer.Dispose();
                return;
            }

            using (Frame frame = new Frame())
            {
                frame.Decode(buffer);
                AmqpTrace.OnFrame(this.Identifier.Value, frame.Type, frame.Channel, frame.Command, false, frame.Size);

                this.heartBeat.OnReceive();
                if (this.UsageMeter != null)
                {
                    this.UsageMeter.OnRead(this, frame.Command != null ? frame.Command.DescriptorCode : 0, buffer.Length);
                }

                if (frame.Command != null)
                {
                    this.ProcessFrame(frame);
                }
            }
        }

        /// <summary>
        /// Handles transport I/O event.
        /// </summary>
        /// <param name="ioEvent">The transport I/O event.</param>
        protected override void HandleIoEvent(IoEvent ioEvent)
        {
            IEnumerator<AmqpSession> it = this.sessionsByLocalHandle.GetSafeEnumerator();
            while (it.MoveNext())
            {
                it.Current.OnIoEvent(ioEvent);
            }
        }

        bool SessionFrameAllowed()
        {
            return this.State == AmqpObjectState.OpenPipe ||
                this.State == AmqpObjectState.OpenSent ||
                this.State == AmqpObjectState.Opened;
        }

        void CloseSessions(bool abort)
        {
            IEnumerable<AmqpSession> sessionSnapshot = null;
            lock (this.ThisLock)
            {
                sessionSnapshot = this.sessionsByLocalHandle.Values;
                if (abort)
                {
                    this.sessionsByLocalHandle.Clear();
                    this.sessionsByRemoteHandle.Clear();
                }
            }

            foreach (AmqpSession session in sessionSnapshot)
            {
                if (abort)
                {
                    session.Abort();
                }
                else
                {
                    session.SafeClose();
                }
            }
        }

        /// <summary>
        /// Process an AMQP frame given to this connection.
        /// </summary>
        /// <param name="frame"></param>
        protected void ProcessFrame(Frame frame)
        {
            Performative command = frame.Command;
            Fx.Assert(command != null, "Must have a valid command");

            if (command.DescriptorCode == OpenCommand.Code)
            {
                this.OnReceiveOpen((Open)frame.Command);
            }
            else if (command.DescriptorCode == CloseCommand.Code)
            {
                this.OnReceiveClose((Close)frame.Command);
            }
            else
            {
                this.OnReceiveSessionFrame(frame);
            }
        }

        void SendProtocolHeader(ProtocolHeader header)
        {
            AmqpTrace.OnProtocolHeader(header, true);
            this.TransitState("S:HDR", StateTransition.SendHeader);
            this.SendDatablock(header);
        }

        void SendOpen()
        {
            this.TransitState("S:OPEN", StateTransition.SendOpen);
            if (this.TerminalException != null)
            {
                this.Settings.AddProperty(AmqpConstants.OpenErrorName, Error.FromException(this.TerminalException));
            }

            if (this.Settings.EnableLinkRecovery)
            {
                if (this.Settings.DesiredCapabilities == null)
                {
                    this.Settings.DesiredCapabilities = new Framing.Multiple<Encoding.AmqpSymbol>();
                }

                this.Settings.DesiredCapabilities.Add(AmqpConstants.LinkRecovery);
            }

            this.SendCommand(this.Settings, 0, null);
        }

        void SendClose()
        {
            this.TransitState("S:CLOSE", StateTransition.SendClose);
            Close close = new Close();
            if (this.TerminalException != null)
            {
                close.Error = Error.FromException(this.TerminalException);
            }

            this.SendCommand(close, 0, null);
        }

        void OnReceiveOpen(Open open)
        {
            StateTransition stateTransition = this.TransitState("R:OPEN", StateTransition.ReceiveOpen);

            uint peerIdleTimeout = open.IdleTimeOut();
            if (peerIdleTimeout < this.Settings.MinIdleTimeout)
            {
                this.CompleteOpen(false,
                    new AmqpException(AmqpErrorCode.NotAllowed, AmqpResources.GetString(AmqpResources.AmqpIdleTimeoutNotSupported, peerIdleTimeout, this.Settings.MinIdleTimeout)));
                return;
            }

            this.Negotiate(open);
            this.NotifyOpening(open);

            if (stateTransition.To == AmqpObjectState.OpenReceived)
            {
                this.SendOpen();
            }

            if(this.isInitiator)
            {
                // check if open returned an error right away
                Error openError = null;
                if (open.Properties != null && open.Properties.TryGetValue<Error>(AmqpConstants.OpenErrorName, out openError))
                {
                    this.CompleteOpen(stateTransition.From == AmqpObjectState.Start, new AmqpException(openError));
                    return;
                }
            }

            uint myIdleTimeout = this.Settings.IdleTimeOut();
            peerIdleTimeout = open.IdleTimeOut();
            if (peerIdleTimeout != uint.MaxValue || myIdleTimeout != uint.MaxValue)
            {
                this.heartBeat = HeartBeat.Initialize(this, myIdleTimeout, peerIdleTimeout);
            }

            this.CompleteOpen(stateTransition.From == AmqpObjectState.Start, null);
        }

        void OnReceiveClose(Close close)
        {
            this.OnReceiveCloseCommand("R:CLOSE", close.Error);
            if (this.State == AmqpObjectState.End)
            {
                this.AsyncIO.SafeClose();
            }
        }

        void OnReceiveSessionFrame(Frame frame)
        {
            AmqpSession session = null;
            Performative command = frame.Command;
            ushort channel = frame.Channel;

            if (command.DescriptorCode == Begin.Code)
            {
                Begin begin = (Begin)command;
                if (begin.RemoteChannel.HasValue)
                {
                    // reply to begin
                    lock (this.ThisLock)
                    {
                        if (!this.sessionsByLocalHandle.TryGetObject(begin.RemoteChannel.Value, out session))
                        {
                            throw new AmqpException(AmqpErrorCode.NotFound, AmqpResources.GetString(AmqpResources.AmqpChannelNotFound, begin.RemoteChannel.Value, this));
                        }

                        session.RemoteChannel = channel;
                        this.sessionsByRemoteHandle.Add(channel, session);
                    }
                }
                else
                {
                    // new begin request
                    AmqpSessionSettings settings = AmqpSessionSettings.Create(begin);
                    settings.RemoteChannel = channel;
                    session = this.SessionFactory.CreateSession(this, settings);
                    this.AddSession(session, channel);
                }
            }
            else
            {
                if (!this.sessionsByRemoteHandle.TryGetObject((uint)channel, out session))
                {
                    if (command.DescriptorCode == End.Code ||
                        command.DescriptorCode == Detach.Code ||
                        this.Settings.IgnoreMissingSessions)
                    {
                        // The session close may timed out already
                        AmqpTrace.Provider.AmqpMissingHandle(this, "session", channel);
                        return;
                    }

                    throw new AmqpException(AmqpErrorCode.NotFound, AmqpResources.GetString(AmqpResources.AmqpChannelNotFound, channel, this));
                }
                else if (command.DescriptorCode == End.Code)
                {
                    this.sessionsByRemoteHandle.Remove((uint)channel);
                    session.RemoteChannel = null;
                }
            }

            session.ProcessFrame(frame);
        }

        void Negotiate(Open open)
        {
            this.Settings.RemoteContainerId = open.ContainerId;
            this.Settings.RemoteHostName = open.HostName;
            this.Settings.ChannelMax = Math.Min(this.Settings.ChannelMax(), open.ChannelMax());
            this.sessionsByLocalHandle.SetMaxHandle(this.Settings.ChannelMax.Value);
            this.sessionsByRemoteHandle.SetMaxHandle(this.Settings.ChannelMax.Value);
            if (open.MaxFrameSize.HasValue)
            {
                this.Settings.MaxFrameSize = Math.Min(this.Settings.MaxFrameSize.Value, open.MaxFrameSize.Value);
            }

            if (open.DesiredCapabilities?.Contains(AmqpConstants.LinkRecovery) == true)
            {
                this.Settings.EnableLinkRecovery = true;
                this.recoverableLinkEndpoints = this.recoverableLinkEndpoints ?? new ConcurrentDictionary<AmqpLinkSettings, AmqpLink>();
                if (this.Settings.OfferedCapabilities == null)
                {
                    this.Settings.OfferedCapabilities = new Multiple<Encoding.AmqpSymbol>();
                }

                this.Settings.OfferedCapabilities.Add(AmqpConstants.LinkRecovery);
            }
        }

        AmqpSession ISessionFactory.CreateSession(AmqpConnection connection, AmqpSessionSettings sessionSettings)
        {
            return new AmqpSession(this, sessionSettings, this.amqpSettings.RuntimeProvider);
        }

        /// <summary>
        /// Adds the session and assigns a local channel number.
        /// </summary>
        /// <param name="session">The session object.</param>
        /// <param name="channel">The remote channel of the session. It is set when the session is created on the listener side.</param>
        public void AddSession(AmqpSession session, ushort? channel)
        {
            lock (this.ThisLock)
            {
                if (this.IsClosing())
                {
                    throw new InvalidOperationException(CommonResources.CreateSessionOnClosingConnection);
                }

                session.Closed += onSessionClosed;
                session.LocalChannel = (ushort)this.sessionsByLocalHandle.Add(session);
                if (channel != null)
                {
                    this.sessionsByRemoteHandle.Add(channel.Value, session);
                }
            }

            AmqpTrace.Provider.AmqpAddSession(this, session, session.LocalChannel, channel ?? 0);
        }

        static void OnSessionClosed(object sender, EventArgs e)
        {
            AmqpSession session = (AmqpSession)sender;
            AmqpConnection thisPtr = session.Connection;
            if (thisPtr != null)
            {
                lock (thisPtr.ThisLock)
                {
                    session.Closed -= onSessionClosed;
                    thisPtr.sessionsByLocalHandle.Remove(session.LocalChannel);
                    if (session.RemoteChannel.HasValue)
                    {
                        thisPtr.sessionsByRemoteHandle.Remove(session.RemoteChannel.Value);
                    }
                }

                AmqpTrace.Provider.AmqpRemoveSession(thisPtr, session, session.LocalChannel, session.RemoteChannel ?? 0);
            }
        }

        abstract class HeartBeat
        {
            public static readonly HeartBeat None = new NoneHeartBeat();

            public abstract void OnSend();

            public abstract void OnReceive();

            public abstract void Stop();

            public static HeartBeat Initialize(AmqpConnection connection, uint local, uint remote)
            {
                return new TimedHeartBeat(connection, local, remote);
            }

            sealed class NoneHeartBeat : HeartBeat
            {
                public override void OnSend()
                {
                }

                public override void OnReceive()
                {
                }

                public override void Stop()
                {
                }
            }

            sealed class TimedHeartBeat : HeartBeat
            {
                readonly AmqpConnection connection;
                readonly uint localInterval;     // idle-timeout for receive (maxValue=infinite)
                readonly uint remoteInterval;    // idle-timeout for send (maxValue=infinite)
                DateTime lastSendTime;
                DateTime lastReceiveTime;
                Timer heartBeatTimer;

                public TimedHeartBeat(AmqpConnection connection, uint local, uint remote)
                {
                    Fx.Assert(local > 0 || remote > 0, "At least one idle timeout must be set");
                    this.connection = connection;
                    this.lastReceiveTime = this.lastSendTime = DateTime.UtcNow;
                    this.localInterval = local;
                    this.remoteInterval = remote < uint.MaxValue ? remote * 7 / 8 : uint.MaxValue;
                    this.heartBeatTimer = new Timer(s => OnHeartBeatTimer(s), this, GetTimerInterval(this.lastSendTime), Timeout.InfiniteTimeSpan);
                }

                public override void OnSend()
                {
                    if (this.remoteInterval < uint.MaxValue)
                    {
                        this.lastSendTime = DateTime.UtcNow;
                    }
                }

                public override void OnReceive()
                {
                    if (this.localInterval < uint.MaxValue)
                    {
                        this.lastReceiveTime = DateTime.UtcNow;
                    }
                }

                public override void Stop()
                {
                    this.heartBeatTimer.Dispose();
                }

                TimeSpan GetTimerInterval(DateTime time)
                {
                    uint remote = GetNextInterval(this.remoteInterval, time, this.lastSendTime);
                    uint local = GetNextInterval(this.localInterval, time, this.lastReceiveTime);
                    uint interval = Math.Min(remote, local);
                    return interval == uint.MaxValue ? Timeout.InfiniteTimeSpan : TimeSpan.FromMilliseconds(interval);
                }

                static uint GetNextInterval(uint interval, DateTime now, DateTime previous)
                {
                    if (interval == uint.MaxValue)
                    {
                        return interval;
                    }

                    uint elapsed = (uint)(now > previous ? (now - previous).TotalMilliseconds : 0.0);
                    return interval > elapsed ? interval - elapsed : 0;
                }

                static void OnHeartBeatTimer(object state)
                {
                    TimedHeartBeat thisPtr = (TimedHeartBeat)state;
                    if (thisPtr.connection.IsClosing())
                    {
                        return;
                    }

                    DateTime now = DateTime.UtcNow;

                    try
                    {
                        if (thisPtr.localInterval < uint.MaxValue &&
                            now.Subtract(thisPtr.lastReceiveTime).TotalMilliseconds > thisPtr.localInterval)
                        {
                            string message = AmqpResources.GetString(AmqpResources.AmqpConnectionInactive,
                                thisPtr.localInterval, thisPtr.connection.Settings.ContainerId);
                            var amqpException = new AmqpException(AmqpErrorCode.ConnectionForced, message);
                            AmqpTrace.Provider.AmqpLogError(thisPtr.connection, "OnHeartBeatTimer", amqpException);
                            thisPtr.connection.SafeClose(amqpException);

                            return;
                        }

                        if (thisPtr.remoteInterval < uint.MaxValue &&
                            now.Subtract(thisPtr.lastSendTime).TotalMilliseconds >= thisPtr.remoteInterval)
                        {
                            thisPtr.connection.SendCommand(null, 0, null);
                        }

                        thisPtr.heartBeatTimer.Change(thisPtr.GetTimerInterval(now), Timeout.InfiniteTimeSpan);
                    }
                    catch (Exception exception) when (!Fx.IsFatal(exception))
                    {
                        if (!thisPtr.connection.IsClosing())
                        {
                            AmqpTrace.Provider.AmqpLogError(thisPtr.connection, "OnHeartBeatTimer", exception);
                            thisPtr.connection.SafeClose(exception);
                        }
                    }
                }
            }
        }
    }
}
