// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
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

        public AmqpConnectionSettings Settings
        {
            get { return this.settings; }
        }

        public IPrincipal Principal
        {
            get { return this.asyncIO.Transport.Principal; }
        }

        public string LocalEndpoint
        {
            get { return this.asyncIO.Transport.LocalEndPoint; }
        }

        public string RemoteEndpoint
        {
            get { return this.asyncIO.Transport.RemoteEndPoint; }
        }

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

        protected AsyncIO AsyncIO
        {
            get { return this.asyncIO; }
        }

        public void SendDatablock(IAmqpSerializable dataBlock)
        {
            ByteBuffer buffer = new ByteBuffer(new byte[dataBlock.EncodeSize]);
            dataBlock.Encode(buffer);

            int size = buffer.Length;
            this.asyncIO.WriteBuffer(buffer);
        }

        public void SendBuffer(ByteBuffer buffer)
        {
            int size = buffer.Length;
            this.asyncIO.WriteBuffer(buffer);
        }

        public void SendBuffers(ByteBuffer[] buffers)
        {
            int totalCount = 0;
            foreach (ByteBuffer buffer in buffers)
            {
                totalCount += buffer.Length;
            }

            this.asyncIO.WriteBuffer(buffers);
        }

        protected abstract void OnProtocolHeader(ProtocolHeader header);

        protected abstract void OnFrameBuffer(ByteBuffer buffer);

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

        void IIoHandler.OnReceiveBuffer(ByteBuffer buffer)
        {
            this.OnReceiveFrameBuffer(buffer);
        }

        void IIoHandler.OnIoFault(Exception exception)
        {
            if (!this.IsClosing())
            {
                AmqpTrace.Provider.AmqpLogError(this, "AsyncIoFault", exception.ToString());
            }

            this.TerminalException = exception;
            this.Abort();
        }

        void IIoHandler.OnIoEvent(IoEvent ioEvent, long queueSize)
        {
            if (!this.IsClosing())
            {
                AmqpTrace.Provider.AmqpIoEvent(this, (int)ioEvent, queueSize);
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
                    AmqpTrace.Provider.AmqpLogError(this, "OnProtocolHeader", exception.Message);

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
                    AmqpTrace.Provider.AmqpLogError(this, "OnFrame", exception.Message);

                    this.SafeClose(exception);
                }
            }
        }
    }
}
