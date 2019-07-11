// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Net;
    using System.Security.Principal;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// The base class for AMQP connection. It should be version independent.
    /// </summary>
    public abstract class AmqpConnectionBase : AmqpObject, IIoHandler, ITransportMonitor
    {
        readonly AmqpConnectionSettings settings;
        readonly AsyncIO asyncIO;
        IAmqpUsageMeter usageMeter;

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="type">A prefix in the object name for debugging related purposes.</param>
        /// <param name="transport">The transport of the connection.</param>
        /// <param name="settings"><see cref="AmqpConnectionSettings"/></param>
        /// <param name="isInitiator">True for transport initiator; false for listener side connections.</param>
        protected AmqpConnectionBase(string type, TransportBase transport, AmqpConnectionSettings settings, bool isInitiator)
            : base(type, transport.Identifier)
        {
            if (settings == null)
            {
                throw new ArgumentNullException(nameof(settings));
            }

            Fx.Assert(transport != null, "transport must not be null.");
            this.settings = settings;
            this.asyncIO = new AsyncIO(this, (int)this.settings.MaxFrameSize(), this.settings.WriteBufferFullLimit,
                this.settings.WriteBufferEmptyLimit, transport, isInitiator);
        }

        /// <summary>
        /// Gets the connection settings.
        /// </summary>
        public AmqpConnectionSettings Settings
        {
            get { return this.settings; }
        }

        /// <summary>
        /// Gets the <see cref="IPrincipal"/> established by the transport.
        /// </summary>
        public IPrincipal Principal
        {
            get { return this.asyncIO.Transport.Principal; }
        }

        /// <summary>
        /// Gets the local endpoint.
        /// </summary>
        public EndPoint LocalEndpoint
        {
            get { return this.asyncIO.Transport.LocalEndPoint; }
        }

        /// <summary>
        /// Gets the remote endpoint.
        /// </summary>
        public EndPoint RemoteEndpoint
        {
            get { return this.asyncIO.Transport.RemoteEndPoint; }
        }

        /// <summary>
        /// Gets or sets a <see cref="IAmqpUsageMeter"/> to monitor incoming and outgoing bytes.
        /// </summary>
        public IAmqpUsageMeter UsageMeter
        {
            get
            {
                return this.usageMeter;
            }

            set
            {
                this.usageMeter = value;
                if (value != null)
                {
                    this.asyncIO.Transport.SetMonitor(this);
                }
            }
        }

        internal AsyncIO AsyncIO
        {
            get { return this.asyncIO; }
        }

        /// <summary>
        /// Sends a serializable object. The buffer is initialized by the
        /// <see cref="IAmqpSerializable.Encode(ByteBuffer)"/> method.
        /// </summary>
        /// <param name="dataBlock">The serializable object.</param>
        public void SendDatablock(IAmqpSerializable dataBlock)
        {
            ByteBuffer buffer = new ByteBuffer(dataBlock.EncodeSize, true);
            dataBlock.Encode(buffer);
            this.asyncIO.WriteBuffer(buffer);
        }

        /// <summary>
        /// Sends a buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        public void SendBuffer(ByteBuffer buffer)
        {
            this.asyncIO.WriteBuffer(buffer);
        }

        /// <summary>
        /// Sends two buffers. The first may be a command and the second may be optional payload.
        /// </summary>
        /// <param name="cmdBuffer">The first buffer.</param>
        /// <param name="payload">The second buffer.</param>
        public void SendBuffer(ByteBuffer cmdBuffer, ByteBuffer payload)
        {
            this.asyncIO.WriteBuffer(cmdBuffer, payload);
        }

        /// <summary>
        /// Called when a protocol header is received.
        /// </summary>
        /// <param name="header">The protocol header.</param>
        protected abstract void OnProtocolHeader(ProtocolHeader header);

        /// <summary>
        /// Called when a frame buffer is received.
        /// </summary>
        /// <param name="buffer">The frame buffer.</param>
        protected abstract void OnFrameBuffer(ByteBuffer buffer);

        /// <summary>
        /// The default handler of I/O events from the transport.
        /// </summary>
        /// <param name="ioEvent">The transport I/O event.</param>
        protected virtual void HandleIoEvent(IoEvent ioEvent)
        {
        }

        void ITransportMonitor.OnTransportWrite(int bufferSize, int writeSize, long queueSize, long latencyTicks)
        {
            this.UsageMeter.OnTransportWrite(bufferSize, writeSize, this.asyncIO.WriteBufferQueueSize, latencyTicks);
        }

        void ITransportMonitor.OnTransportRead(int bufferSize, int readSize, int cacheHits, long latencyTicks)
        {
            this.UsageMeter.OnTransportRead(bufferSize, readSize, cacheHits, latencyTicks);
        }

        ByteBuffer IIoHandler.CreateBuffer(int frameSize)
        {
            var buffer = new ByteBuffer(frameSize, false);
            AmqpBitConverter.WriteUInt(buffer, (uint)frameSize);
            return buffer;
        }

        void IIoHandler.OnReceiveBuffer(ByteBuffer buffer)
        {
            this.OnReceiveFrameBuffer(buffer);
        }

        void IIoHandler.OnIoFault(Exception exception)
        {
            if (!this.IsClosing())
            {
                AmqpTrace.Provider.AmqpLogError(this, "AsyncIoFault", exception);
            }

            this.TerminalException = exception;
            this.Abort();
        }

        void IIoHandler.OnIoEvent(IoEvent ioEvent, long queueSize)
        {
            if (!this.IsClosing())
            {
                AmqpTrace.Provider.AmqpIoEvent(this, ioEvent, queueSize);
                this.HandleIoEvent(ioEvent);
            }
        }

        void OnReceiveFrameBuffer(ByteBuffer buffer)
        {
            if (this.State <= AmqpObjectState.OpenClosePipe)
            {
                Fx.Assert(buffer.Length == AmqpConstants.ProtocolHeaderSize, "protocol header size is wrong");
                try
                {
                    ProtocolHeader header = new ProtocolHeader();
                    header.Decode(buffer);
                    this.OnProtocolHeader(header);
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    AmqpTrace.Provider.AmqpLogError(this, "OnProtocolHeader", exception);

                    this.TerminalException = exception;
                    this.Abort();
                }
            }
            else
            {
                try
                {
                    this.OnFrameBuffer(buffer);
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    AmqpTrace.Provider.AmqpLogError(this, "OnFrame", exception);

                    this.SafeClose(exception);
                }
            }
        }
    }
}
