﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
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
        IAmqpUsageMeter usageMeter;
        HeartBeat heartBeat;
        KeyedByTypeCollection<object> extensions;

        public AmqpConnection(TransportBase transport, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this(transport, amqpSettings.GetDefaultHeader(), true, amqpSettings, connectionSettings)
        {
        }

        public AmqpConnection(TransportBase transport, ProtocolHeader protocolHeader, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this(transport, protocolHeader, true, amqpSettings, connectionSettings)
        {
        }

        public AmqpConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            base((isInitiator ? "out" : "in") + "-connection", transport, connectionSettings, isInitiator)
        {
            if (amqpSettings == null)
            {
                throw new ArgumentNullException("amqpSettings");
            }

            this.initialHeader = protocolHeader;
            this.isInitiator = isInitiator;
            this.amqpSettings = amqpSettings;
            this.sessionsByLocalHandle = new HandleTable<AmqpSession>(this.Settings.ChannelMax ?? AmqpConstants.DefaultMaxConcurrentChannels - 1);
            this.sessionsByRemoteHandle = new HandleTable<AmqpSession>(this.Settings.ChannelMax ?? AmqpConstants.DefaultMaxConcurrentChannels - 1);
            this.SessionFactory = this;
            this.heartBeat = HeartBeat.None;
        }

        public AmqpSettings AmqpSettings
        {
            get { return this.amqpSettings; }
        }

        public ISessionFactory SessionFactory
        {
            get;
            set;
        }

        public IAmqpUsageMeter UsageMeter
        {
            get { return this.usageMeter; }
            set { this.usageMeter = value; }
        }

        public KeyedByTypeCollection<object> Extensions
        {
            get
            {
                return LazyInitializer.EnsureInitialized(ref this.extensions);
            }
        }

        public bool IsInitiator
        {
            get { return this.isInitiator; }
        }

        public object SessionLock
        {
            get { return this.ThisLock; }
        }

        public AmqpSession CreateSession(AmqpSessionSettings sessionSettings)
        {
            if (this.IsClosing())
            {
                throw new InvalidOperationException(CommonResources.CreateSessionOnClosingConnection);
            }

            AmqpSession session = this.SessionFactory.CreateSession(this, sessionSettings);
            this.AddSession(session, null);
            return session;
        }

        public void SendCommand(Performative command, ushort channel, ArraySegment<byte>[] payload)
        {
#if DEBUG
            Frame frame = new Frame();
            frame.Channel = channel;
            frame.Command = command;
            frame.Trace(true);
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Send, frame);
#endif

            int frameSize = 0;
            if (payload == null)
            {
                // The frame buffer is disposed when the write completes
                ByteBuffer buffer = Frame.EncodeCommand(FrameType.Amqp, channel, command, 0);
                frameSize = buffer.Length;
                this.SendBuffer(buffer);
            }
            else
            {
                ByteBuffer[] buffers = new ByteBuffer[1 + payload.Length];
                int payloadSize = 0;
                for (int i = 0; i < payload.Length; ++i)
                {
                    ArraySegment<byte> segment = payload[i];
                    payloadSize += segment.Count;
                    buffers[i + 1] = new ByteBuffer(segment);
                }

                // The frame buffer is disposed when the write completes
                ByteBuffer cmdBuffer = Frame.EncodeCommand(FrameType.Amqp, channel, command, payloadSize);
                frameSize = cmdBuffer.Length + payloadSize;
                buffers[0] = cmdBuffer;
                this.SendBuffers(buffers);
            }

            this.heartBeat.OnSend();
            if (this.usageMeter != null)
            {
                this.usageMeter.OnWrite(this, command == null ? 0 : command.DescriptorCode, frameSize);
            }
        }

        protected override bool OpenInternal()
        {
            AmqpTrace.Provider.AmqpOpenConnection(this, this);
            if (this.isInitiator)
            {
                this.AsyncIO.Open();
                this.SendProtocolHeader(this.initialHeader);
                this.SendOpen();
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

        protected override void AbortInternal()
        {
            AmqpTrace.Provider.AmqpCloseConnection(this, this, true);
            this.heartBeat.Stop();
            this.CloseSessions(true);
            this.AsyncIO.Abort();
        }

        protected override void OnProtocolHeader(ProtocolHeader header)
        {
#if DEBUG
            header.Trace(false);
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Receive, header);
#endif
            this.heartBeat.OnReceive();
            if (this.usageMeter != null)
            {
                this.usageMeter.OnRead(this, 0, header.EncodeSize);
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
#if DEBUG
                frame.Trace(false);
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Receive, frame);
#endif

                this.heartBeat.OnReceive();
                if (this.usageMeter != null)
                {
                    this.usageMeter.OnRead(this, frame.Command != null ? frame.Command.DescriptorCode : 0, buffer.Length);
                }

                if (frame.Command != null)
                {
                    this.ProcessFrame(frame);
                }
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

        void ProcessFrame(Frame frame)
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
#if DEBUG
            header.Trace(true);
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Send, header);
#endif
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
            this.Negotiate(open);
            this.NotifyOpening(open);

            uint peerIdleTimeout = open.IdleTimeOut();
            if (peerIdleTimeout < AmqpConstants.MinimumHeartBeatIntervalMs)
            {
                this.CompleteOpen(false,
                    new AmqpException(AmqpErrorCode.NotAllowed, AmqpResources.GetString(AmqpResources.AmqpIdleTimeoutNotSupported, peerIdleTimeout, AmqpConstants.MinimumHeartBeatIntervalMs)));
                return;
            }

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

            if (peerIdleTimeout != uint.MaxValue)
            {
                this.heartBeat = HeartBeat.Initialize(this, peerIdleTimeout);
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
            if (this.isInitiator)
            {
                this.FindMutualCapabilites(this.Settings.DesiredCapabilities, open.OfferedCapabilities);
            }
            else
            {
                this.FindMutualCapabilites(this.Settings.OfferedCapabilities, open.DesiredCapabilities);
            }
            
            if (open.MaxFrameSize.HasValue)
            {
                this.Settings.MaxFrameSize = Math.Min(this.Settings.MaxFrameSize.Value, open.MaxFrameSize.Value);
            }
        }

        AmqpSession ISessionFactory.CreateSession(AmqpConnection connection, AmqpSessionSettings sessionSettings)
        {
            return new AmqpSession(this, sessionSettings, this.amqpSettings.RuntimeProvider);
        }

        public void AddSession(AmqpSession session, ushort? channel)
        {
            session.Closed += onSessionClosed;
            lock (this.ThisLock)
            {
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
                    thisPtr.sessionsByLocalHandle.Remove(session.LocalChannel);
                    if (session.RemoteChannel.HasValue)
                    {
                        thisPtr.sessionsByRemoteHandle.Remove(session.RemoteChannel.Value);
                    }
                }

                AmqpTrace.Provider.AmqpRemoveSession(thisPtr, session, session.LocalChannel, session.CachedRemoteChannel);
            }
        }
        
        abstract class HeartBeat
        {
            public static readonly HeartBeat None = new NoneHeartBeat();

            public abstract void OnSend();

            public abstract void OnReceive();

            public abstract void Stop();

            public static HeartBeat Initialize(AmqpConnection connection, uint interval)
            {
                Fx.Assert(interval < uint.MaxValue, "invalid interval");
                return new TimedHeartBeat(connection, interval);
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
                readonly Timer heartBeatTimer;
                readonly int heartBeatInterval;
                DateTime lastSendTime;
                DateTime lastReceiveTime;

                public TimedHeartBeat(AmqpConnection connection, uint interval)
                {
                    this.connection = connection;
                    this.lastReceiveTime = DateTime.UtcNow;
                    this.lastSendTime = DateTime.UtcNow;
                    this.heartBeatInterval = (int)(interval * 7 / 8);
                    this.heartBeatTimer = new Timer(OnHeartBeatTimer, this, this.heartBeatInterval, Timeout.Infinite);
                }

                public override void OnSend()
                {
                    this.lastSendTime = DateTime.UtcNow;
                }

                public override void OnReceive()
                {
                    this.lastReceiveTime = DateTime.UtcNow;
                }

                public override void Stop()
                {
                    this.heartBeatTimer.Change(Timeout.Infinite, Timeout.Infinite);
                }

                static void OnHeartBeatTimer(object state)
                {
                    TimedHeartBeat thisPtr = (TimedHeartBeat)state;
                    if (thisPtr.connection.IsClosing())
                    {
                        return;
                    }

                    bool wasActive = false;
                    DateTime capturedLastActive = thisPtr.lastSendTime;
                    DateTime now = DateTime.UtcNow;
                    DateTime scheduleAfterTime = now;
                    if (now.Subtract(capturedLastActive) < TimeSpan.FromMilliseconds(thisPtr.heartBeatInterval))
                    {
                        wasActive = true;
                        scheduleAfterTime = capturedLastActive;
                    }

                    try
                    {
                        if (!wasActive)
                        {
                            thisPtr.connection.SendCommand(null, 0, null);
                        }

                        thisPtr.heartBeatTimer.Change(scheduleAfterTime.AddMilliseconds(thisPtr.heartBeatInterval).Subtract(now), Timeout.InfiniteTimeSpan);
                    }
                    catch (Exception exception)
                    {
                        if (Fx.IsFatal(exception))
                        {
                            throw;
                        }

                        AmqpTrace.Provider.AmqpLogError(thisPtr.connection, "OnHeartBeatTimer", exception.Message);
                    }

                    // idle timeout can be different for the peers. but instead of creating another timer,
                    // we use the sending timer to also check if we have received anything just in case
                    uint thisIdletimeout = thisPtr.connection.Settings.IdleTimeOut();
                    if (thisIdletimeout < uint.MaxValue &&
                        now > thisPtr.lastReceiveTime.AddMilliseconds((int)thisIdletimeout))
                    {
                        AmqpTrace.Provider.AmqpLogError(thisPtr.connection, "OnHeartBeatTimer", AmqpResources.GetString(AmqpResources.AmqpTimeout, thisIdletimeout, thisPtr.connection));

                        thisPtr.connection.SafeClose(new AmqpException(AmqpErrorCode.ConnectionForced, AmqpResources.AmqpConnectionInactive));
                    }
                }
            }
        }
    }
}
