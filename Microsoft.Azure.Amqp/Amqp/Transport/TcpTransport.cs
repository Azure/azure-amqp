// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net.Sockets;
    using Microsoft.Azure.Amqp.Encoding;

    sealed class TcpTransport : TransportBase
    {
        static readonly SegmentBufferPool SmallBufferPool = new SegmentBufferPool(FixedWidth.ULong, 100000);
        static readonly EventHandler<SocketAsyncEventArgs> onWriteComplete = OnWriteComplete;
        static readonly EventHandler<SocketAsyncEventArgs> onReadComplete = OnReadComplete;
        readonly Socket socket;
        readonly string localEndPoint;
        readonly string remoteEndPoint;
        readonly WriteAsyncEventArgs sendEventArgs;
        readonly ReadAsyncEventArgs receiveEventArgs;
        ITransportMonitor monitor;

        public TcpTransport(Socket socket, TcpTransportSettings transportSettings)
            : base("tcp")
        {
            this.socket = socket;
            this.socket.NoDelay = true;
            this.localEndPoint = this.socket.LocalEndPoint.ToString();
            this.remoteEndPoint = this.socket.RemoteEndPoint.ToString();
            this.sendEventArgs = new WriteAsyncEventArgs(transportSettings.SendBufferSize);
            this.sendEventArgs.Transport = this;
            this.sendEventArgs.Completed += onWriteComplete;
            this.receiveEventArgs = new ReadAsyncEventArgs(transportSettings.ReceiveBufferSize);
            this.receiveEventArgs.Completed += onReadComplete;
            this.receiveEventArgs.Transport = this;
            if (transportSettings.InternalSendBufferSize >= 0)
            {
                this.socket.SendBufferSize = transportSettings.InternalSendBufferSize;
            }
            if (transportSettings.InternalReceiveBufferSize >= 0)
            {
                this.socket.ReceiveBufferSize = transportSettings.InternalReceiveBufferSize;
            }
        }

        public override string LocalEndPoint
        {
            get
            {
                return this.localEndPoint;
            }
        }

        public override string RemoteEndPoint
        {
            get
            {
                return this.remoteEndPoint;
            }
        }

        public override void SetMonitor(ITransportMonitor monitor)
        {
            this.monitor = monitor;
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

            ByteBuffer readBuffer = this.receiveEventArgs.PrepareRead(args.Count);
            if (readBuffer != null)
            {
                // ensure the buffer is not reclaimed while read is pending
                // ref count is decremented in read complete handler
                this.receiveEventArgs.UserToken = readBuffer.AddReference();

                if (readBuffer.Length > 0)
                {
                    this.HandleReadComplete(args, true, true);
                    return false;
                }

                this.receiveEventArgs.SetBuffer(readBuffer.Buffer, readBuffer.Offset, readBuffer.Size);
            }
            else
            {
                this.receiveEventArgs.SetReadBuffer(args);
            }

            this.receiveEventArgs.Args = args;
            Fx.Assert(this.receiveEventArgs.Count > 0, "Must have a count to read");
            bool pending;
            try
            {
                pending = this.socket.ReceiveAsync(this.receiveEventArgs);
            }
            catch
            {
                if (readBuffer != null)
                {
                    readBuffer.Dispose();
                }

                throw;
            }

            if (!pending)
            {
                this.HandleReadComplete(args, false, true);
                return false;
            }

            return true;
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
                if (this.monitor != null)
                {
                    this.sendEventArgs.ReportWrite(this.monitor);
                }
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
            ByteBuffer readBuffer = this.receiveEventArgs.UserToken as ByteBuffer;
            try
            {
                if (this.receiveEventArgs.SocketError == SocketError.Success)
                {
                    int bytesCopied = 0;
                    if (this.receiveEventArgs.IsSegment)
                    {
                        bytesCopied = this.receiveEventArgs.BytesTransferred;
                        Buffer.BlockCopy(this.receiveEventArgs.Buffer, this.receiveEventArgs.Offset,
                            args.Buffer, args.Offset, bytesCopied);
                    }
                    else
                    {
                        if (readBuffer != null)
                        {
                            int bytesTransferred;
                            if (fromCache)
                            {
                                bytesTransferred = readBuffer.Length;
                            }
                            else
                            {
                                bytesTransferred = this.receiveEventArgs.BytesTransferred;
                                readBuffer.Append(bytesTransferred);
                                if (this.monitor != null)
                                {
                                    this.receiveEventArgs.ReportRead(this.monitor);
                                }
                            }

                            if (bytesTransferred > 0)
                            {
                                if (bytesTransferred <= args.Count)
                                {
                                    bytesCopied = bytesTransferred;
                                    Buffer.BlockCopy(readBuffer.Buffer, readBuffer.Offset, args.Buffer, args.Offset, bytesTransferred);
                                    readBuffer.Reset();
                                }
                                else
                                {
                                    bytesCopied = args.Count;
                                    Buffer.BlockCopy(readBuffer.Buffer, readBuffer.Offset, args.Buffer, args.Offset, bytesCopied);
                                    readBuffer.Complete(bytesCopied);
                                }
                            }
                        }
                        else
                        {
                            bytesCopied = this.receiveEventArgs.BytesTransferred;
                        }
                    }

                    args.BytesTransfered = bytesCopied;
                    args.Exception = null;
                }
                else
                {
                    args.Exception = new SocketException((int)this.receiveEventArgs.SocketError);
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                args.Exception = exception;
            }
            finally
            {
                if (readBuffer != null)
                {
                    // ref count was incremented when read starts
                    readBuffer.Dispose();
                }
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
            readonly BufferSizeTracker writeTracker;
            Timestamp startTime;
            int bufferSize;

            public WriteAsyncEventArgs(int bufferSize)
            {
                this.bufferSize = bufferSize;
                if (bufferSize == 0)
                {
                    this.writeTracker = new BufferSizeTracker(1024);
                }
            }

            public TcpTransport Transport { get; set; }

            public TransportAsyncCallbackArgs Args { get; set; }

            public void PrepareWrite(int writeSize)
            {
                this.startTime = Timestamp.Now;

                int newSize;
                if (this.writeTracker != null &&
                    this.writeTracker.TryUpdateBufferSize(writeSize, out newSize))
                {
                    AmqpTrace.Provider.AmqpDynamicBufferSizeChange(this.Transport, "write", this.bufferSize, newSize);

                    this.bufferSize = newSize;
                    this.Transport.socket.SendBufferSize = this.bufferSize;
                }
            }

            public void ReportWrite(ITransportMonitor monitor)
            {
                monitor.OnTransportWrite(this.bufferSize, this.BytesTransferred, 0, this.startTime.ElapsedTicks);
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
            readonly BufferSizeTracker readTracker;
            int bufferSize;
            ArraySegment<byte> segment; // read small buffers when bufferSize is 0
            ByteBuffer readBuffer;
            Timestamp startTime;
            int cacheHits;

            public ReadAsyncEventArgs(int bufferSize)
            {
                this.bufferSize = bufferSize;
                this.segment = SmallBufferPool.TakeBuffer(FixedWidth.ULong);
                if (bufferSize == 0)
                {
                    this.readTracker = new BufferSizeTracker(512);
                }
                else
                {
                    this.readBuffer = new ByteBuffer(bufferSize, false, true);
                }
            }

            public TcpTransport Transport { get; set; }

            public TransportAsyncCallbackArgs Args { get; set; }

            public bool IsSegment { get; set; }

            public ByteBuffer PrepareRead(int count)
            {
                int newSize;
                if (this.readTracker != null &&
                    this.readTracker.TryUpdateBufferSize(count, out newSize))
                {
                    AmqpTrace.Provider.AmqpDynamicBufferSizeChange(this.Transport, "read", this.bufferSize, newSize);

                    this.bufferSize = newSize;
                    this.Transport.socket.ReceiveBufferSize = this.bufferSize;
                }

                ByteBuffer current = this.readBuffer;
                if (current == null)
                {
                    this.startTime = Timestamp.Now;
                    if (this.bufferSize > 0)
                    {
                        current = new ByteBuffer(this.bufferSize, false, true);
                        this.readBuffer = current;
                    }
                }
                else
                {
                    if (current.Length == 0)
                    {
                        this.startTime = Timestamp.Now;
                        if (this.bufferSize == 0)
                        {
                            current.Dispose();
                            this.readBuffer = null;
                            current = null;
                        }
                    }
                    else
                    {
                        this.cacheHits++;
                    }
                }

                return current;
            }

            public void ReportRead(ITransportMonitor monitor)
            {
                monitor.OnTransportRead(this.bufferSize, this.BytesTransferred, this.cacheHits, this.startTime.ElapsedTicks);
                this.cacheHits = 0;
            }

            public void SetReadBuffer(TransportAsyncCallbackArgs args)
            {
                // for most idle connections, the read is pending on reading the frame size
                // use the segment buffer to avoid heap fragmentation
                if (args.Count <= SmallBufferPool.SegmentSize)
                {
                    Fx.AssertAndThrow(this.segment.Array != null, "segment buffer already relaimed");
                    this.SetBuffer(this.segment.Array, this.segment.Offset, args.Count);
                    this.IsSegment = true;
                }
                else
                {
                    this.SetBuffer(args.Buffer, args.Offset, args.Count);
                }
            }

            public void Reset()
            {
                this.IsSegment = false;
                this.Args = null;
                this.SetBuffer(null, 0, 0);
                this.UserToken = null;
            }

            public new void Dispose()
            {
                ByteBuffer temp = this.readBuffer;
                if (temp != null)
                {
                    temp.Dispose();
                }

                ArraySegment<byte> copy = this.segment;
                if (copy.Array != null)
                {
                    this.segment = default(ArraySegment<byte>);
                    SmallBufferPool.ReturnBuffer(copy);
                }

                base.Dispose();
            }
        }

        sealed class BufferSizeTracker
        {
            // level 0: for idle connections (mostly heartbeats)
            // level 1: active connections (constant I/O activities)
            // level 2: busy connections (high throughput)
            // unitSize is the min value to increase to help small messages to reach level 1
            // level is changed only when the trend is consistent during two consecutive windows
            // bufferSizes: must match the preallocated buffers in InternalBufferManager.PreallocatedBufferManager
            static long durationTicks = TimeSpan.FromSeconds(4).Ticks;
            static int[] thresholds = new int[] { 0, 8 * 1024, 4 * 1024 * 1024 };
            static int[] bufferSizes = new int[] { 0, 8 * 1024, 64 * 1024 };
            int unitSize;
            DateTime firstOperation;
            int transferedBytes;
            sbyte level;
            sbyte direction;

            public BufferSizeTracker(int unitSize)
            {
                this.unitSize = unitSize;
                this.firstOperation = DateTime.UtcNow;
            }

            public bool TryUpdateBufferSize(int bytes, out int bufferSize)
            {
                this.transferedBytes += Math.Max(bytes, this.unitSize);

                bufferSize = 0;

                int newLevel = 0;
                bool levelChanged = false;
                DateTime now = DateTime.UtcNow;
                if (now.Ticks - this.firstOperation.Ticks >= durationTicks)
                {
                    for (int i = thresholds.Length - 1; i >= 0; --i)
                    {
                        if (this.transferedBytes >= thresholds[i])
                        {
                            newLevel = i;
                            break;
                        }
                    }

                    if (newLevel > this.level)
                    {
                        if (this.direction > 0)
                        {
                            this.level++;
                            bufferSize = bufferSizes[this.level];
                            levelChanged = true;
                        }
                        else
                        {
                            this.direction = 1;
                        }
                    }
                    else if (newLevel < this.level)
                    {
                        if (this.direction < 0)
                        {
                            this.level--;
                            bufferSize = bufferSizes[this.level];
                            levelChanged = true;
                        }
                        else
                        {
                            this.direction = -1;
                        }
                    }
                    else
                    {
                        this.direction = 0;
                    }

                    this.transferedBytes = 0;
                    this.firstOperation = now;
                }

                return levelChanged;
            }
        }
    }
}
