// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Runtime.InteropServices.WindowsRuntime;
    using System.Threading.Tasks;
    using Windows.Networking.Sockets;
    using Windows.Storage.Streams;

    public class WebSocketTransport : TransportBase
    {
        readonly StreamWebSocket webSocket;
        readonly Uri uri;
        ITransportMonitor usageMeter;

        public override EndPoint LocalEndPoint
        {
            get
            {
                return new IPEndPoint(IPAddress.Any, -1);
            }
        }

        public override EndPoint RemoteEndPoint
        {
            get
            {
                return new DnsEndPoint(this.uri.Host, this.uri.Port);
            }
        }

        internal WebSocketTransport(StreamWebSocket webSocket, Uri uri)
            : base(WebSocketTransportSettings.WebSockets)
        {
            this.webSocket = webSocket;
            this.uri = uri;
        }

        public sealed override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            ArraySegment<byte> segment;
            ByteBuffer mergedBuffer = null;
            DateTime startTime = DateTime.UtcNow;
            if (args.Buffer != null)
            {
                segment = new ArraySegment<byte>(args.Buffer, args.Offset, args.Count);
            }
            else
            {
                Fx.Assert(args.ByteBufferList != null, "Buffer list should not be null when buffer is null");
                if (args.ByteBufferList.Count == 1)
                {
                    ByteBuffer byteBuffer = args.ByteBufferList[0];
                    segment = new ArraySegment<byte>(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
                }
                else
                {
                    // Copy all buffers into one big buffer to avoid SSL overhead
                    mergedBuffer = new ByteBuffer(args.Count, false, false);
                    for (int i = 0; i < args.ByteBufferList.Count; ++i)
                    {
                        ByteBuffer byteBuffer = args.ByteBufferList[i];
                        System.Buffer.BlockCopy(byteBuffer.Buffer, byteBuffer.Offset,
                            mergedBuffer.Buffer, mergedBuffer.Length, byteBuffer.Length);
                        mergedBuffer.Append(byteBuffer.Length);
                    }

                    segment = new ArraySegment<byte>(mergedBuffer.Buffer, 0, mergedBuffer.Length);
                }
            }

            Task task = this.webSocket.OutputStream.WriteAsync(segment.Array.AsBuffer(segment.Offset, segment.Count)).AsTask();
            if (task.IsCompleted)
            {
                this.OnWriteComplete(args, segment, mergedBuffer, startTime);
                return false;
            }

            task.ContinueWith(t =>
            {
                if (t.IsFaulted)
                {
                    args.Exception = t.Exception.InnerException;
                }
                else if (t.IsCanceled)
                {
                    args.Exception = new OperationCanceledException();
                }
                else
                {
                    this.OnWriteComplete(args, segment, mergedBuffer, startTime);
                }

                args.CompletedCallback(args);
            });
            return true;
        }

        public sealed override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            DateTime startTime = DateTime.UtcNow;
            IBuffer buffer = args.Buffer.AsBuffer(args.Offset, args.Count);
            var task = this.webSocket.InputStream.ReadAsync(buffer, (uint)args.Count, InputStreamOptions.None).AsTask();
            if (task.IsCompleted)
            {
                this.OnReadComplete(args, (int)task.Result.Length, startTime);
                return false;
            }

            task.ContinueWith(t =>
            {
                if (t.IsFaulted)
                {
                    args.Exception = t.Exception.InnerException;
                }
                else if (t.IsCanceled)
                {
                    args.Exception = new OperationCanceledException();
                }
                else
                {
                    this.OnReadComplete(args, (int)t.Result.Length, startTime);
                }

                args.CompletedCallback(args);
            });
            return true;
        }

        protected override bool OpenInternal()
        {
            return true;
        }

        protected override bool CloseInternal()
        {
            this.webSocket.Close(0, "close");
            return true;
        }

        protected override void AbortInternal()
        {
            this.webSocket.Dispose();
        }

        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.usageMeter = usageMeter;
        }

        void OnWriteComplete(TransportAsyncCallbackArgs args, ArraySegment<byte> buffer, ByteBuffer byteBuffer, DateTime startTime)
        {
            args.BytesTransfered = buffer.Count;
            if (byteBuffer != null)
            {
                byteBuffer.Dispose();
            }

            if (this.usageMeter != null)
            {
                this.usageMeter.OnTransportWrite(0, buffer.Count, 0, DateTime.UtcNow.Subtract(startTime).Ticks);
            }
        }

        void OnReadComplete(TransportAsyncCallbackArgs args, int count, DateTime startTime)
        {
            args.BytesTransfered = count;
            if (this.usageMeter != null)
            {
                this.usageMeter.OnTransportRead(0, count, 0, DateTime.UtcNow.Subtract(startTime).Ticks);
            }
        }
    }
}