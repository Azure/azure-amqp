// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Transport;

    [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
        Justification = "Uses custom scheme for cleanup")]
    public sealed class AsyncIO : AmqpObject
    {
        readonly IIoHandler ioHandler;
        readonly TransportBase transport;
        readonly AsyncWriter writer;
        readonly AsyncReader reader;

        public AsyncIO(IIoHandler parent, int maxFrameSize, int writeQueueFullLimit,
            int writeQueueEmptyLimit, TransportBase transport, bool isInitiator)
            : base("async-io", transport.Identifier)
        {
            Fx.Assert(transport != null, "transport required");
            this.ioHandler = parent;
            this.transport = transport;
            this.writer = this.transport.RequiresCompleteFrames ?
                new AsyncFrameWriter(this.transport, writeQueueFullLimit, writeQueueEmptyLimit, parent) :
                new AsyncWriter(this.transport, writeQueueFullLimit, writeQueueEmptyLimit, parent);
            this.reader = new AsyncReader(this, maxFrameSize, isInitiator);
        }

        public TransportBase Transport
        {
            get { return this.transport; }
        }

        public long WriteBufferQueueSize
        {
            get { return this.writer.BufferQueueSize; }
        }

        public void WriteBuffer(ByteBuffer buffer)
        {
            this.writer.WriteBuffer(buffer);
        }

        public void WriteBuffer(IList<ByteBuffer> buffers)
        {
            this.writer.WriteBuffer(buffers);
        }

        protected override void OnOpen(TimeSpan timeout)
        {
            this.OpenInternal();
            this.State = AmqpObjectState.Opened;
        }

        protected override void OnClose(TimeSpan timeout)
        {
            this.writer.IssueClose();
        }

        protected override bool OpenInternal()
        {
            this.reader.StartReading();
            return true;
        }

        protected override bool CloseInternal()
        {
            this.writer.IssueClose();
            return true;
        }

        protected override void AbortInternal()
        {
            this.transport.Abort();
        }

        /// <summary>
        /// A reader that pumps frames from the transport and hands it over to the handler
        /// </summary>
        [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
           Justification = "Uses custom protocol for cleanup")]
        sealed class AsyncReader
        {
            enum ReadState : byte
            {
                ProtocolHeader,
                FrameSize,
                FrameBody
            }

            static readonly SegmentBufferPool FrameSizeSegmentPool = new SegmentBufferPool(FixedWidth.UInt, 100000);
            static readonly Action<TransportAsyncCallbackArgs> onReadBufferComplete = OnReadBufferComplete;
            readonly AsyncIO asyncIo;
            readonly int maxFrameSize;
            readonly bool readProtocolHeader;
            readonly TransportAsyncCallbackArgs readAsyncEventArgs;
            ReadState readState;
            ArraySegment<byte> frameSizeBuffer;
            int remainingBytes;

            public AsyncReader(AsyncIO parent, int maxFrameSize, bool readProtocolHeader)
            {
                this.asyncIo = parent;
                this.maxFrameSize = maxFrameSize;
                this.readProtocolHeader = readProtocolHeader;
                this.frameSizeBuffer = FrameSizeSegmentPool.TakeBuffer(FixedWidth.UInt);
                this.readAsyncEventArgs = new TransportAsyncCallbackArgs();
                this.readAsyncEventArgs.CompletedCallback = onReadBufferComplete;
                this.readAsyncEventArgs.UserToken = this;
            }

            public void StartReading()
            {
                if (this.readProtocolHeader)
                {
                    // read reply header first.
                    this.SetReadProtocolHeader();
                }
                else
                {
                    this.SetReadFrameSize();
                }

                this.ReadBuffer();
            }

            void Cleanup()
            {
                ArraySegment<byte> temp;
                lock (this.readAsyncEventArgs)
                {
                    temp = this.frameSizeBuffer;
                    this.frameSizeBuffer = default(ArraySegment<byte>);
                }

                if (temp.Array != null)
                {
                    FrameSizeSegmentPool.ReturnBuffer(temp);
                }
            }

            void SetReadProtocolHeader()
            {
                this.readState = ReadState.ProtocolHeader;
                byte[] headerBuffer = new byte[AmqpConstants.ProtocolHeaderSize];
                this.remainingBytes = headerBuffer.Length;
                this.readAsyncEventArgs.SetBuffer(headerBuffer, 0, headerBuffer.Length);
            }

            void SetReadFrameSize()
            {
                Fx.Assert(this.frameSizeBuffer.Array != null, "frame size buffer should be initialized already");
                Fx.Assert(this.frameSizeBuffer.Count == FixedWidth.UInt, "wrong buffer size");
                this.readState = ReadState.FrameSize;
                this.remainingBytes = FixedWidth.UInt;
                this.readAsyncEventArgs.SetBuffer(this.frameSizeBuffer.Array, this.frameSizeBuffer.Offset, this.frameSizeBuffer.Count);
                this.readAsyncEventArgs.UserToken2 = null;
            }

            void SetReadFrameBody(int frameSize)
            {
                ByteBuffer buffer = new ByteBuffer(frameSize, false, false);
                AmqpBitConverter.WriteUInt(buffer, (uint)frameSize);

                this.readState = ReadState.FrameBody;
                this.remainingBytes = buffer.Size;
                this.readAsyncEventArgs.SetBuffer(buffer.Buffer, buffer.Length, this.remainingBytes);
                this.readAsyncEventArgs.UserToken2 = buffer;
            }

            static void OnReadBufferComplete(TransportAsyncCallbackArgs args)
            {
                var thisPtr = (AsyncReader)args.UserToken;
                bool shouldContine;
                try
                {
                    shouldContine = thisPtr.HandleReadBufferComplete(args);
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    thisPtr.asyncIo.ioHandler.OnIoFault(exception.ToIOException());
                    thisPtr.Cleanup();
                    shouldContine = false;
                }

                if (shouldContine)
                {
                    thisPtr.ReadBuffer();
                }
            }

            void ReadBuffer()
            {
                try
                {
                    if (this.asyncIo.State == AmqpObjectState.End)
                    {
                        this.Cleanup();
                        ByteBuffer buffer = (ByteBuffer)this.readAsyncEventArgs.UserToken2;
                        if (buffer != null)
                        {
                            buffer.Dispose();
                        }
                    }
                    else
                    {
                        while (this.asyncIo.State != AmqpObjectState.End)
                        {
                            if (!this.asyncIo.transport.ReadAsync(this.readAsyncEventArgs))
                            {
                                if (!this.HandleReadBufferComplete(this.readAsyncEventArgs))
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    this.asyncIo.ioHandler.OnIoFault(exception.ToIOException());
                    this.Cleanup();
                }
            }

            bool HandleReadBufferComplete(TransportAsyncCallbackArgs args)
            {
                bool shouldContinue;
                if (args.Exception != null)
                {
                    this.asyncIo.ioHandler.OnIoFault(args.Exception.ToIOException());
                    shouldContinue = false;
                }
                else if (args.BytesTransfered == 0)
                {
                    // connection closed by other side
                    this.asyncIo.ioHandler.OnIoFault(new AmqpException(AmqpErrorCode.ConnectionForced, null));
                    shouldContinue = false;
                }
                else
                {
                    Fx.Assert(this.remainingBytes == args.Count, "remaining bytes should be the same as args.Count");
                    Fx.Assert(this.remainingBytes >= args.BytesTransfered, "remaining bytes cannot be less than args.BytesTransfered " + this.remainingBytes);
                    shouldContinue = true;
                    this.remainingBytes -= args.BytesTransfered;

                    if (this.remainingBytes > 0)
                    {
                        // Buffer partially completed.
                        Fx.Assert(args.BytesTransfered < args.Count, "bytes transfered should be less than count");
                        args.SetBuffer(args.Buffer, args.Offset + args.BytesTransfered, args.Count - args.BytesTransfered);
                    }
                    else
                    {
                        Fx.Assert(this.remainingBytes == 0, "remaining bytes should be 0");
                        switch (this.readState)
                        {
                            case ReadState.FrameBody:
                                this.HandleFrameBodyReadComplete(args);
                                break;
                            case ReadState.FrameSize:
                                this.HandleFrameSizeReadComplete(args);
                                break;
                            case ReadState.ProtocolHeader:
                                this.HandleProtocolHeaderReadComplete(args);
                                break;
                            default:
                                throw new AmqpException(AmqpErrorCode.IllegalState, null);
                        }
                    }
                }

                if (!shouldContinue)
                {
                    this.Cleanup();
                }

                return shouldContinue;
            }

            void HandleProtocolHeaderReadComplete(TransportAsyncCallbackArgs args)
            {
                this.asyncIo.ioHandler.OnReceiveBuffer(new ByteBuffer(args.Buffer, 0, AmqpConstants.ProtocolHeaderSize));

                this.SetReadFrameSize();
            }

            void HandleFrameSizeReadComplete(TransportAsyncCallbackArgs args)
            {
                Fx.Assert(args.Buffer == this.frameSizeBuffer.Array, "wrong buffer");
                int frameSize = (int)AmqpBitConverter.ReadUInt(this.frameSizeBuffer.Array, this.frameSizeBuffer.Offset, FixedWidth.UInt);
                Fx.Assert(frameSize > 0, "frameSize must be positive");
                if (frameSize <= 0 || frameSize > this.maxFrameSize)
                {
                    throw new AmqpException(AmqpErrorCode.FramingError, CommonResources.GetString(CommonResources.InvalidFrameSize, frameSize, this.maxFrameSize));
                }

                this.SetReadFrameBody(frameSize);
            }

            void HandleFrameBodyReadComplete(TransportAsyncCallbackArgs args)
            {
                ByteBuffer buffer = (ByteBuffer)args.UserToken2;
                Fx.Assert(buffer != null, "Buffer must not be null when reading body");
                buffer.Append(buffer.Size);
                Fx.Assert(buffer.Size == 0, "Buffer should be filled entirely");
                this.asyncIo.ioHandler.OnReceiveBuffer(buffer);

                this.SetReadFrameSize();
            }
        }

        /// <summary>
        /// A reader that reads specified bytes and notifies caller upon completion (pull).
        /// </summary>
        public class AsyncBufferReader
        {
            static Action<TransportAsyncCallbackArgs> onReadComplete = OnReadComplete;
            readonly TransportBase transport;

            public AsyncBufferReader(TransportBase transport)
            {
                this.transport = transport;
            }

            public void ReadBuffer(TransportAsyncCallbackArgs args)
            {
                TransportAsyncCallbackArgs wrapperArgs = new TransportAsyncCallbackArgs();
                wrapperArgs.SetBuffer(args.Buffer, args.Offset, args.Count);
                wrapperArgs.UserToken = this;
                wrapperArgs.UserToken2 = args;
                wrapperArgs.CompletedCallback = AsyncBufferReader.onReadComplete;
                this.Read(wrapperArgs);
            }

            void Read(TransportAsyncCallbackArgs args)
            {
                try
                {
                    while (true)
                    {
                        if (this.transport.ReadAsync(args))
                        {
                            break;
                        }

                        if (this.HandleReadComplete(args))
                        {
                            break;
                        }
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    args.Exception = exception;
                    this.HandleReadComplete(args);
                }
            }

            bool HandleReadComplete(TransportAsyncCallbackArgs args)
            {
                bool done = true;
                Exception exception = null;
                if (args.Exception != null)
                {
                    exception = args.Exception;
                }
                else if (args.BytesTransfered == 0)
                {
                    exception = new ObjectDisposedException(this.transport.ToString());
                }
                else if (args.BytesTransfered < args.Count)
                {
                    args.SetBuffer(args.Buffer, args.Offset + args.BytesTransfered, args.Count - args.BytesTransfered);
                    done = false;
                }

                if (done)
                {
                    TransportAsyncCallbackArgs innerArgs = (TransportAsyncCallbackArgs)args.UserToken2;
                    innerArgs.Exception = exception;
                    innerArgs.BytesTransfered = innerArgs.Count;
                    innerArgs.CompletedCallback(innerArgs);
                }

                return done;
            }

            static void OnReadComplete(TransportAsyncCallbackArgs args)
            {
                AsyncBufferReader thisPtr = (AsyncBufferReader)args.UserToken;
                if (!thisPtr.HandleReadComplete(args) && !args.CompletedSynchronously)
                {
                    thisPtr.Read(args);
                }
            }
        }
        
        /// <summary>
        /// A reader that reads AMQP frame buffers. Not thread safe.
        /// </summary>
        public sealed class FrameBufferReader
        {
            static readonly Action<TransportAsyncCallbackArgs> onSizeComplete = OnReadSizeComplete;
            static readonly Action<TransportAsyncCallbackArgs> onFrameComplete = OnReadFrameComplete;
            readonly TransportBase transport;
            readonly int maxFrameSize;
            readonly byte[] sizeBuffer;
            IIoHandler parent;

            public FrameBufferReader(IIoHandler parent, TransportBase transport, int maxFrameSize)
            {
                this.parent = parent;
                this.transport = transport;
                this.maxFrameSize = maxFrameSize;
                this.sizeBuffer = new byte[FixedWidth.UInt];
            }

            public void ReadFrame()
            {
                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.UserToken = this;
                args.SetBuffer(this.sizeBuffer, 0, this.sizeBuffer.Length);
                args.CompletedCallback = onSizeComplete;
                this.ReadCore(args);
            }

            void ReadCore(TransportAsyncCallbackArgs args)
            {
                try
                {
                    while (!this.transport.ReadAsync(args))
                    {
                        if (this.HandleReadComplete(args))
                        {
                            break;
                        }
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    args.Exception = exception;
                    this.HandleReadComplete(args);
                }
            }

            static void OnReadSizeComplete(TransportAsyncCallbackArgs args)
            {
                FrameBufferReader thisPtr = (FrameBufferReader)args.UserToken;
                if (!thisPtr.HandleReadComplete(args))
                {
                    thisPtr.ReadCore(args);
                }
            }

            static void OnReadFrameComplete(TransportAsyncCallbackArgs args)
            {
                FrameBufferReader thisPtr = (FrameBufferReader) args.UserToken;
                if (!thisPtr.HandleReadComplete(args))
                {
                    thisPtr.ReadCore(args);
                }
            }

            bool HandleReadComplete(TransportAsyncCallbackArgs args)
            {
                bool completed = true;
                Exception exception = null;
                if (args.Exception != null)
                {
                    exception = args.Exception;
                }
                else if (args.BytesTransfered == 0)
                {
                    exception = new ObjectDisposedException(this.transport.ToString());
                }
                else if (args.BytesTransfered < args.Count)
                {
                    args.SetBuffer(args.Buffer, args.Offset + args.BytesTransfered, args.Count - args.BytesTransfered);
                    completed = false;
                }

                if (completed)
                {
                    if (exception != null || object.ReferenceEquals(args.CompletedCallback, onFrameComplete))
                    {
                        ByteBuffer buffer = null;
                        if (exception == null)
                        {
                            buffer = new ByteBuffer(args.Buffer, 0, args.Buffer.Length);
                            this.parent.OnReceiveBuffer(buffer);
                        }
                        else
                        {
                            this.parent.OnIoFault(exception.ToIOException());
                        }
                    }
                    else
                    {
                        // read size completed ok
                        uint size = AmqpBitConverter.ReadUInt(this.sizeBuffer, 0, this.sizeBuffer.Length);
                        if (size > this.maxFrameSize)
                        {
                            completed = true;
                            exception = new AmqpException(AmqpErrorCode.FramingError, CommonResources.GetString(CommonResources.InvalidFrameSize, size, this.maxFrameSize));
                            this.parent.OnIoFault(exception);
                        }
                        else
                        {
                            byte[] frameBuffer = new byte[size];
                            Buffer.BlockCopy(this.sizeBuffer, 0, frameBuffer, 0, this.sizeBuffer.Length);
                            args.SetBuffer(frameBuffer, this.sizeBuffer.Length, (int)size - this.sizeBuffer.Length);
                            args.CompletedCallback = onFrameComplete;
                            completed = false;
                        }
                    }
                }

                return completed;
            }
        }

        /// <summary>
        /// A writer that writes buffers. Buffer writes may be batched. Writer owns closing the transport.
        /// </summary>
        public class AsyncWriter
        {
            const int MaxBatchSize = 32 * 1024;
            static readonly Action<TransportAsyncCallbackArgs> writeCompleteCallback = WriteCompleteCallback;
            readonly TransportBase transport;
            readonly int writeQueueFullLimit;
            readonly int writeQueueEmptyLimit;
            readonly TransportAsyncCallbackArgs writeAsyncEventArgs;
            readonly Queue<ByteBuffer> bufferQueue;
            readonly IIoHandler parent;
            long bufferQueueSize;
            bool writing;
            bool closed;
            bool isQueueFull;

            public AsyncWriter(TransportBase transport, int writeQueueFullLimit, int writeQueueEmptyLimit, IIoHandler parent)
            {
                this.transport = transport;
                this.writeQueueFullLimit = writeQueueFullLimit;
                this.writeQueueEmptyLimit = writeQueueEmptyLimit;
                this.parent = parent;
                this.bufferQueue = new Queue<ByteBuffer>();
                this.writeAsyncEventArgs = new TransportAsyncCallbackArgs();
                this.writeAsyncEventArgs.CompletedCallback = writeCompleteCallback;
            }

            public long BufferQueueSize
            {
                get { return this.bufferQueueSize; }
            }

            object SyncRoot
            {
                get { return this.bufferQueue; }
            }

            public void WriteBuffer(ByteBuffer buffer)
            {
                lock (this.SyncRoot)
                {
                    if (this.writing)
                    {
                        this.EnqueueBuffer(buffer);
                        return;
                    }

                    this.writing = true;
                }

                this.writeAsyncEventArgs.SetBuffer(buffer);
                if (this.WriteCore())
                {
                    this.ContinueWrite();
                }
            }

            public virtual void WriteBuffer(IList<ByteBuffer> buffers)
            {
                lock (this.SyncRoot)
                {
                    if (this.writing)
                    {
                        for (int i = 0; i < buffers.Count; i++)
                        {
                            this.EnqueueBuffer(buffers[i]);
                        }

                        return;
                    }

                    this.writing = true;
                }

                this.writeAsyncEventArgs.SetBuffer(buffers);
                if (this.WriteCore())
                {
                    this.ContinueWrite();
                }
            }

            public void IssueClose()
            {
                lock (this.SyncRoot)
                {
                    this.closed = true;
                    if (this.writing)
                    {
                        return;
                    }
                }

                this.transport.SafeClose();
            }

            static void WriteCompleteCallback(TransportAsyncCallbackArgs args)
            {
                AsyncWriter thisPtr = (AsyncWriter)args.UserToken;
                if (!thisPtr.HandleWriteBufferComplete(args))
                {
                    return;
                }

                thisPtr.ContinueWrite();
            }

            // call this under lock
            void EnqueueBuffer(ByteBuffer buffer)
            {
                this.bufferQueueSize += buffer.Length;
                this.bufferQueue.Enqueue(buffer);
                if (this.bufferQueueSize >= this.writeQueueFullLimit && !this.isQueueFull)
                {
                    this.isQueueFull = true;
                    this.parent.OnIoEvent(IoEvent.WriteBufferQueueFull, this.bufferQueueSize);
                }
            }

            // call this under lock
            void OnBufferDequeued(int size)
            {
                this.bufferQueueSize -= size;
                if (this.bufferQueueSize <= this.writeQueueEmptyLimit && this.isQueueFull)
                {
                    this.isQueueFull = false;
                    this.parent.OnIoEvent(IoEvent.WriteBufferQueueEmpty, this.bufferQueueSize);
                }
            }

            bool WriteCore()
            {
                Fx.Assert(this.writeAsyncEventArgs.Buffer != null || this.writeAsyncEventArgs.ByteBufferList != null, "No buffer(s) set");
                try
                {
                    this.writeAsyncEventArgs.UserToken = this;
                    if (this.transport.WriteAsync(this.writeAsyncEventArgs))
                    {
                        // write is pending
                        return false;
                    }
                    else
                    {
                        // completed synchronously
                        Fx.Assert(this.writeAsyncEventArgs.BytesTransfered > 0 || this.writeAsyncEventArgs.Exception != null, "no bytes sent on success");
                        return this.HandleWriteBufferComplete(this.writeAsyncEventArgs);
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    this.writeAsyncEventArgs.Exception = exception;
                    this.writeAsyncEventArgs.UserToken = this;
                    return this.HandleWriteBufferComplete(this.writeAsyncEventArgs);
                }
            }

            void ContinueWrite()
            {
                do
                {
                    lock (this.SyncRoot)
                    {
                        int count = this.bufferQueue.Count;
                        if (count == 0)
                        {
                            this.writing = false;
                            if (this.closed)
                            {
                                this.transport.SafeClose();
                            }

                            return;
                        }
                        else if (count == 1)
                        {
                            ByteBuffer buffer = this.bufferQueue.Dequeue();
                            this.OnBufferDequeued(buffer.Length);
                            this.writeAsyncEventArgs.SetBuffer(buffer);
                        }
                        else
                        {
                            var buffers = new List<ByteBuffer>(Math.Min(count, 64));
                            int size = 0;
                            for (int i = 0; i < count && size < MaxBatchSize; i++)
                            {
                                ByteBuffer buffer = this.bufferQueue.Dequeue();
                                buffers.Add(buffer);
                                size += buffer.Length;
                            }

                            this.OnBufferDequeued(size);
                            this.writeAsyncEventArgs.SetBuffer(buffers);
                        }
                    }
                }
                while (this.WriteCore());
            }

            bool HandleWriteBufferComplete(TransportAsyncCallbackArgs args)
            {
                bool shouldContinue;
                if (args.Exception != null)
                {
                    shouldContinue = false;
                    this.parent.OnIoFault(args.Exception.ToIOException());
                }
                else
                {
                    Fx.Assert(args.BytesTransfered == args.Count, "Bytes transferred not equal to the bytes set.");
                    shouldContinue = true;
                }

                args.Reset();
                return shouldContinue;
            }
        }

        /// <summary>
        /// A writer that writes complete AMQP frames. Frame writes may be batched. Writer owns closing the transport.
        /// </summary>
        public class AsyncFrameWriter : AsyncWriter
        {
            public AsyncFrameWriter(TransportBase transport, int writeQueueFullLimit, int writeQueueEmptyLimit, IIoHandler parent)
                : base(transport, writeQueueFullLimit, writeQueueEmptyLimit, parent)
            {
            }

            public override void WriteBuffer(IList<ByteBuffer> buffers)
            {
                Fx.Assert(buffers.Count > 0, "buffers.Count should be set");
                int count = 0;
                foreach (ByteBuffer byteBuffer in buffers)
                {
                    count += byteBuffer.Length;
                }

                ByteBuffer bigBuffer = new ByteBuffer(count, false, false);
                foreach (ByteBuffer byteBuffer in buffers)
                {
                    Buffer.BlockCopy(byteBuffer.Buffer, byteBuffer.Offset, bigBuffer.Buffer, bigBuffer.Length, byteBuffer.Length);
                    bigBuffer.Append(byteBuffer.Length);

                    // Dispose incoming frame buffers since the caller is expecting us to release these objects
                    byteBuffer.Dispose();
                }

                base.WriteBuffer(bigBuffer);
            }
        }

        /// <summary>
        /// A writer that writes fixed-size buffer and notify caller upon completion.
        /// </summary>
        public class AsyncBufferWriter
        {
            readonly TransportBase transport;
            static Action<TransportAsyncCallbackArgs> onWriteComplete = OnWriteComplete;

            public AsyncBufferWriter(TransportBase transport)
            {
                this.transport = transport;
            }

            public void WriteBuffer(TransportAsyncCallbackArgs args)
            {
                TransportAsyncCallbackArgs wrapperArgs = new TransportAsyncCallbackArgs();
                wrapperArgs.SetBuffer(args.Buffer, args.Offset, args.Count);
                wrapperArgs.CompletedCallback = AsyncBufferWriter.onWriteComplete;
                wrapperArgs.UserToken = this;
                wrapperArgs.UserToken2 = args;
                this.Write(wrapperArgs);
            }

            void Write(TransportAsyncCallbackArgs args)
            {
                try
                {
                    while (true)
                    {
                        if (this.transport.WriteAsync(args))
                        {
                            break;
                        }

                        if (this.HandleWriteComplete(args))
                        {
                            break;
                        }
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    args.Exception = exception;
                    this.HandleWriteComplete(args);
                }
            }

            bool HandleWriteComplete(TransportAsyncCallbackArgs args)
            {
                bool done = true;
                Exception exception = null;
                if (args.Exception != null)
                {
                    exception = args.Exception;
                }
                else if (args.BytesTransfered == 0)
                {
                    exception = new ObjectDisposedException(this.transport.ToString());
                }
                else if (args.BytesTransfered < args.Count)
                {
                    args.SetBuffer(args.Buffer, args.Offset + args.BytesTransfered, args.Count - args.BytesTransfered);
                    done = false;
                }

                TransportAsyncCallbackArgs innerArgs = (TransportAsyncCallbackArgs)args.UserToken2;
                if (done && innerArgs.CompletedCallback != null)
                {
                    innerArgs.Exception = exception;
                    innerArgs.BytesTransfered = innerArgs.Count;
                    innerArgs.CompletedCallback(innerArgs);
                }

                return done;
            }

            static void OnWriteComplete(TransportAsyncCallbackArgs args)
            {
                AsyncBufferWriter thisPtr = (AsyncBufferWriter)args.UserToken;
                if (!thisPtr.HandleWriteComplete(args) && !args.CompletedSynchronously)
                {
                    thisPtr.Write(args);
                }
            }
        }
    }
}
