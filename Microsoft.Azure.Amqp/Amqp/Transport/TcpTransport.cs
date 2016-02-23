// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Net;
    using System.Net.Sockets;
    using Microsoft.Azure.Amqp.Encoding;

    [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
        Justification = "Uses custom scheme for cleanup")]
    sealed class TcpTransport : TransportBase
    {
        static readonly SegmentBufferPool SmallBufferPool = new SegmentBufferPool(FixedWidth.ULong, 100000);
        static readonly EventHandler<SocketAsyncEventArgs> onWriteComplete = OnWriteComplete;
        static readonly EventHandler<SocketAsyncEventArgs> onReadComplete = OnReadComplete;
        readonly Socket socket;
        readonly EndPoint localEndPoint;
        readonly EndPoint remoteEndPoint;
        readonly WriteAsyncEventArgs sendEventArgs;
        readonly ReadAsyncEventArgs receiveEventArgs;

        public TcpTransport(Socket socket, TcpTransportSettings transportSettings)
            : base("tcp")
        {
            this.socket = socket;
            this.socket.NoDelay = true;
            this.socket.SendBufferSize = 0;
            this.socket.ReceiveBufferSize = 0;
            this.localEndPoint = this.socket.LocalEndPoint;
            this.remoteEndPoint = this.socket.RemoteEndPoint;
            this.sendEventArgs = new WriteAsyncEventArgs();
            this.sendEventArgs.Transport = this;
            this.sendEventArgs.Completed += onWriteComplete;
            this.receiveEventArgs = new ReadAsyncEventArgs();
            this.receiveEventArgs.Completed += onReadComplete;
            this.receiveEventArgs.Transport = this;
        }

        public override EndPoint LocalEndPoint
        {
            get
            {
                return this.localEndPoint;
            }
        }

        public override EndPoint RemoteEndPoint
        {
            get
            {
                return this.remoteEndPoint;
            }
        }

        public sealed override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(args.Buffer != null || args.ByteBufferList != null, "must have a buffer or buffers to write");
            Fx.Assert(args.CompletedCallback != null, "must have a valid callback");
            Fx.Assert(args.BytesTransfered == 0, "args.BytesTransfered != 0");
            Fx.Assert(this.sendEventArgs.Args == null, "write is pending");

            this.sendEventArgs.PrepareWrite(args.Count);

            if (args.Buffer != null)
            {
                this.sendEventArgs.SetBuffer(args.Buffer, args.Offset, args.Count);
            }
            else
            {
                ArraySegment<byte>[] buffers = new ArraySegment<byte>[args.ByteBufferList.Count];
                for (int i = 0; i < buffers.Length; ++i)
                {
                    buffers[i] = new ArraySegment<byte>(args.ByteBufferList[i].Buffer, args.ByteBufferList[i].Offset, args.ByteBufferList[i].Length);
                }

                this.sendEventArgs.BufferList = buffers;
            }

            this.sendEventArgs.Args = args;
            if (!this.socket.SendAsync(this.sendEventArgs))
            {
                this.HandleWriteComplete(args, true);
                return false;
            }

            return true;
        }

        public sealed override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(args.Buffer != null, "must have buffer(s) to read");
            Fx.Assert(args.CompletedCallback != null, "must have a valid callback");
            Fx.Assert(this.receiveEventArgs.Args == null, "read is pending");

            this.receiveEventArgs.AddBytes(args.Count);

            bool pending;
            if (this.receiveEventArgs.ReadBuffer != null && this.receiveEventArgs.ReadBuffer.Length > 0)
            {
                this.HandleReadComplete(args, true, true);
                pending = false;
            }
            else
            {
                if (args.Count <= SmallBufferPool.SegmentSize)
                {
                    ArraySegment<byte> smallSegment = SmallBufferPool.TakeBuffer(args.Count);
                    this.receiveEventArgs.SetBuffer(smallSegment.Array, smallSegment.Offset, smallSegment.Count);
                    this.receiveEventArgs.IsSegment = true;
                }
                else
                {
                    this.receiveEventArgs.PrepareRead(args.Count);
                    ByteBuffer buffer = this.receiveEventArgs.ReadBuffer;
                    Fx.Assert(buffer != null, "read buffer should be initialized");
                    this.receiveEventArgs.SetBuffer(buffer.Buffer, buffer.Offset, buffer.Size);
                }

                this.receiveEventArgs.Args = args;
                Fx.Assert(this.receiveEventArgs.Count > 0, "Must have a count to read");
                if (!this.socket.ReceiveAsync(this.receiveEventArgs))
                {
                    this.HandleReadComplete(args, false, true);
                    pending = false;
                }
                else
                {
                    pending = true;
                }
            }

            return pending;
        }

        protected override bool CloseInternal()
        {
            try
            {
                this.socket.Shutdown(SocketShutdown.Both);
                this.socket.Dispose();
            }
            finally
            {
                // dispose receive args after closing socket since we cannot
                // return the receive buffer while the socket is still active
                this.sendEventArgs.Dispose();
                this.receiveEventArgs.Dispose();
            }

            return true;
        }

        protected override void AbortInternal()
        {
            this.socket.Dispose();
            this.sendEventArgs.Dispose();
            this.receiveEventArgs.Dispose();
        }

        static void OnWriteComplete(object sender, SocketAsyncEventArgs socketArgs)
        {
            WriteAsyncEventArgs socketArgsEx = (WriteAsyncEventArgs)socketArgs;
            socketArgsEx.Transport.HandleWriteComplete(socketArgsEx.Args, false);
        }

        static void OnReadComplete(object sender, SocketAsyncEventArgs socketArgs)
        {
            ReadAsyncEventArgs socketArgsEx = (ReadAsyncEventArgs)socketArgs;
            socketArgsEx.Transport.HandleReadComplete(socketArgsEx.Args, false, false);
        }

        void HandleWriteComplete(TransportAsyncCallbackArgs args, bool syncCompleted)
        {
            if (this.sendEventArgs.SocketError == SocketError.Success)
            {
                args.BytesTransfered = this.sendEventArgs.BytesTransferred;
                args.Exception = null;
                Fx.Assert(args.BytesTransfered == args.Count, "Cannot be partialy completed");
            }
            else
            {
                args.Exception = new SocketException((int)this.sendEventArgs.SocketError);
            }

            args.CompletedSynchronously = syncCompleted;

            try
            {
                this.sendEventArgs.Reset();
            }
            catch (ObjectDisposedException exception)
            {
                args.Exception = exception;
            }

            if (!syncCompleted)
            {
                args.CompletedCallback(args);
            }
        }

        void HandleReadComplete(TransportAsyncCallbackArgs args, bool fromCache, bool completedSynchronously)
        {
            if (this.receiveEventArgs.SocketError == SocketError.Success)
            {
                try
                {
                    int bytesTransferred = fromCache ? this.receiveEventArgs.ReadBuffer.Length : this.receiveEventArgs.BytesTransferred;
                    int bytesCopied = bytesTransferred;
                    if (bytesTransferred > 0)
                    {
                        if (this.receiveEventArgs.IsSegment)
                        {
                            Fx.Assert(bytesTransferred <= args.Count, "Should not receive more than requested");
                            Buffer.BlockCopy(this.receiveEventArgs.Buffer, this.receiveEventArgs.Offset,
                                args.Buffer, args.Offset, bytesTransferred);
                            var segment = new ArraySegment<byte>(this.receiveEventArgs.Buffer,
                                this.receiveEventArgs.Offset, this.receiveEventArgs.Count);
                            SmallBufferPool.ReturnBuffer(segment);
                        }
                        else
                        {
                            ByteBuffer buffer = this.receiveEventArgs.ReadBuffer;
                            if (buffer == null)
                            {
                                // transport is disposed
                                bytesCopied = 0;
                            }
                            else
                            {
                                if (!fromCache)
                                {
                                    buffer.Append(bytesTransferred);
                                }

                                if (bytesTransferred <= args.Count)
                                {
                                    Buffer.BlockCopy(buffer.Buffer, buffer.Offset, args.Buffer, args.Offset, bytesTransferred);
                                    buffer.Reset();
                                }
                                else
                                {
                                    Buffer.BlockCopy(buffer.Buffer, buffer.Offset, args.Buffer, args.Offset, args.Count);
                                    bytesCopied = args.Count;
                                    buffer.Complete(args.Count);
                                }
                            }
                        }
                    }

                    args.BytesTransfered = bytesCopied;
                    args.Exception = null;
                }
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    args.Exception = exception;
                }
            }
            else
            {
                args.Exception = new SocketException((int)this.receiveEventArgs.SocketError);
            }

            args.CompletedSynchronously = completedSynchronously;

            try
            {
                this.receiveEventArgs.Reset();
            }
            catch (ObjectDisposedException exception)
            {
                args.Exception = exception;
            }

            if (!completedSynchronously)
            {
                args.CompletedCallback(args);
            }
        }

        sealed class WriteAsyncEventArgs : SocketAsyncEventArgs
        {
            BufferSizeTracker writeTracker;
            private int bufferSize;

            public TcpTransport Transport { get; set; }

            public TransportAsyncCallbackArgs Args { get; set; }

            public void PrepareWrite(int writeSize)
            {
                this.writeTracker.AddBytes(writeSize);

                int newSize;
                if (this.writeTracker.TryUpdateBufferSize(out newSize))
                {
                    AmqpTrace.Provider.AmqpDynamicBufferSizeChange(this.Transport, "write", this.bufferSize, newSize);

                    this.bufferSize = newSize;
                    this.Transport.socket.SendBufferSize = this.bufferSize;
                }
            }

            public void Reset()
            {
                this.Args = null;
                this.SetBuffer(null, 0, 0);
                this.BufferList = null;
            }
        }

        sealed class ReadAsyncEventArgs : SocketAsyncEventArgs
        {
            BufferSizeTracker readTracker;
            int bufferSize;

            public TcpTransport Transport { get; set; }

            public TransportAsyncCallbackArgs Args { get; set; }

            public ByteBuffer ReadBuffer { get; set; }

            public bool IsSegment { get; set; }

            public void AddBytes(int bytes)
            {
                this.readTracker.AddBytes(bytes);
            }

            public void PrepareRead(int count)
            {
                // for read, the bytes is already added
                int newSize;
                if (this.readTracker.TryUpdateBufferSize(out newSize))
                {
                    AmqpTrace.Provider.AmqpDynamicBufferSizeChange(this.Transport, "read", this.bufferSize, newSize);

                    this.bufferSize = newSize;
                    this.Transport.socket.ReceiveBufferSize = this.bufferSize;
                    if (this.ReadBuffer != null)
                    {
                        this.ReadBuffer.Dispose();
                        this.ReadBuffer = null;
                    }
                }

                if (this.ReadBuffer == null)
                {
                    int size = this.bufferSize > 0 ? this.bufferSize : Math.Min(count, AmqpConstants.TransportBufferSize);
                    this.ReadBuffer = new ByteBuffer(size, false, true);
                }
            }

            public void Reset()
            {
                this.IsSegment = false;
                this.Args = null;
                this.SetBuffer(null, 0, 0);
                if (this.bufferSize == 0 && this.ReadBuffer != null)
                {
                    this.ReadBuffer.Dispose();
                    this.ReadBuffer = null;
                }
            }

            public new void Dispose()
            {
                if (this.ReadBuffer != null)
                {
                    this.ReadBuffer.Dispose();
                    this.ReadBuffer = null;
                }

                base.Dispose();
            }
        }

        struct BufferSizeTracker
        {
            static long durationTicks = TimeSpan.FromSeconds(5).Ticks;
            static int[] thresholds = new int[] { 0, 64 * 1024, 512 * 1024, 2 * 1024 * 1024 };
            static int[] bufferSizes = new int[] { 0, 4 * 1024, 16 * 1024, 64 * 1024 };
            Timestamp firstOperation;
            int transferedBytes;
            int level;

            public void AddBytes(int bytes)
            {
                if (this.transferedBytes == 0)
                {
                    firstOperation = Timestamp.Now;
                }

                this.transferedBytes += bytes;
            }

            public bool TryUpdateBufferSize(out int bufferSize)
            {
                bufferSize = 0;

                int newLevel = 0;
                bool levelChanged = false;
                long elapsedTicks = this.firstOperation.ElapsedTicks;
                if (elapsedTicks >= durationTicks)
                {
                    for (int i = thresholds.Length - 1; i >= 0; --i)
                    {
                        if (this.transferedBytes >= thresholds[i])
                        {
                            newLevel = i;
                            break;
                        }
                    }

                    this.transferedBytes = 0;
                    if (newLevel != this.level)
                    {
                        this.level = newLevel;
                        bufferSize = bufferSizes[newLevel];
                        levelChanged = true;
                    }
                }

                return levelChanged;
            }
        }
    }
}
