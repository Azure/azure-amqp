// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
#if NET45
    using System;
    using System.Net;
    using System.Net.WebSockets;
    using System.Threading;
    using System.Threading.Tasks;

    public class WebSocketTransport : TransportBase
    {
        internal const string WebSocketSubProtocol = "amqp";
        internal const string WebSockets = "ws";
        internal const string SecureWebSockets = "wss";
        internal const int WebSocketsPort = 80;
        internal const int SecureWebSocketsPort = 443;
        readonly WebSocket webSocket;
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

        internal WebSocketTransport(WebSocket webSocket, Uri uri)
            : base(WebSockets)
        {
            this.webSocket = webSocket;
            this.uri = uri;
        }

        public sealed override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            ArraySegment<byte> buffer;
            ByteBuffer mergedBuffer = null;
            DateTime startTime = DateTime.UtcNow;
            if (args.Buffer != null)
            {
                buffer = new ArraySegment<byte>(args.Buffer, args.Offset, args.Count);
            }
            else
            {
                Fx.Assert(args.ByteBufferList != null, "Buffer list should not be null when buffer is null");
                if (args.ByteBufferList.Count == 1)
                {
                    ByteBuffer byteBuffer = args.ByteBufferList[0];
                    buffer = new ArraySegment<byte>(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
                }
                else
                {
                    // Copy all buffers into one big buffer to avoid SSL overhead
                    mergedBuffer = new ByteBuffer(args.Count, false, false);
                    for (int i = 0; i < args.ByteBufferList.Count; ++i)
                    {
                        ByteBuffer byteBuffer = args.ByteBufferList[i];
                        Buffer.BlockCopy(byteBuffer.Buffer, byteBuffer.Offset, mergedBuffer.Buffer, mergedBuffer.Length, byteBuffer.Length);
                        mergedBuffer.Append(byteBuffer.Length);
                    }

                    buffer = new ArraySegment<byte>(mergedBuffer.Buffer, 0, mergedBuffer.Length);
                }
            }

            Task task = this.webSocket.SendAsync(buffer, WebSocketMessageType.Binary, true, CancellationToken.None);
            if (task.IsCompleted)
            {
                this.OnWriteComplete(args, buffer, mergedBuffer, startTime);
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
                    this.OnWriteComplete(args, buffer, mergedBuffer, startTime);
                }

                args.CompletedCallback(args);
            });
            return true;
        }

        public sealed override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            DateTime startTime = DateTime.UtcNow;
            ArraySegment<byte> buffer = new ArraySegment<byte>(args.Buffer, args.Offset, args.Count);
            Task<WebSocketReceiveResult> task = this.webSocket.ReceiveAsync(buffer, CancellationToken.None);
            if (task.IsCompleted)
            {
                this.OnReadComplete(args, task.Result.Count, startTime);
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
                    this.OnReadComplete(args, t.Result.Count, startTime);
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
            Task task = webSocket.CloseAsync(WebSocketCloseStatus.Empty, string.Empty, CancellationToken.None);
            if (task.IsCompleted)
            {
                return false;
            }

            task.ContinueWith(t =>
            {
                Exception exception = t.IsFaulted ? t.Exception.InnerException : (t.IsCanceled ? new OperationCanceledException() : null);
                this.CompleteClose(false, exception);
            });
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

        internal static bool MatchScheme(string scheme)
        {
            return string.Equals(scheme, WebSockets, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(scheme, SecureWebSockets, StringComparison.OrdinalIgnoreCase);
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
#endif
}