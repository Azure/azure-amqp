// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Net.WebSockets;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Defines the web socket transport.
    /// </summary>
    public class WebSocketTransport : TransportBase
    {
        readonly WebSocket webSocket;
        readonly Uri uri;
        readonly EndPoint local;
        readonly EndPoint remote;
        ITransportMonitor usageMeter;

        /// <inheritdoc cref="TransportBase.Local"/>
        internal override EndPoint Local => this.local;

        /// <inheritdoc cref="TransportBase.Remote"/>
        internal override EndPoint Remote => this.remote;

        /// <inheritdoc/>
        public override string LocalEndPoint => string.Empty;

        /// <inheritdoc/>
        public override string RemoteEndPoint => this.uri?.OriginalString;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="webSocket">The web socket.</param>
        /// <param name="uri">The URI.</param>
        [Obsolete("Obsolete")]
        public WebSocketTransport(WebSocket webSocket, Uri uri)
            : base(WebSocketTransportSettings.WebSockets)
        {
            this.webSocket = webSocket;
            this.uri = uri;
        }

        internal WebSocketTransport(WebSocket webSocket, Uri uri, EndPoint local, EndPoint remote)
            : base(WebSocketTransportSettings.WebSockets)
        {
            this.webSocket = webSocket;
            this.uri = uri;
            this.local = local;
            this.remote = remote;
        }

        /// <summary>
        /// Starts a write operation.
        /// </summary>
        /// <param name="args">The write arguments.</param>
        /// <returns>true if the write operation is pending, otherwise false.</returns>
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
                this.OnWriteComplete(args, task, buffer, mergedBuffer, startTime);
                return false;
            }

            task.ContinueWith(static (t, s) =>
            {
                var (transport, args, buffer, mergedBuffer, startTime) = (Tuple<WebSocketTransport, TransportAsyncCallbackArgs, ArraySegment<byte>, ByteBuffer, DateTime>) s;
                transport.OnWriteComplete(args, t, buffer, mergedBuffer, startTime);
                args.CompletedCallback(args);
            }, Tuple.Create(this, args, buffer, mergedBuffer, startTime));
            return true;
        }

        /// <summary>
        /// Starts a read operation.
        /// </summary>
        /// <param name="args">The read arguments.</param>
        /// <returns>true if the read operation is pending, otherwise false.</returns>
        public sealed override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            DateTime startTime = DateTime.UtcNow;
            ArraySegment<byte> buffer = new ArraySegment<byte>(args.Buffer, args.Offset, args.Count);
            Task<WebSocketReceiveResult> task = this.webSocket.ReceiveAsync(buffer, CancellationToken.None);
            if (task.IsCompleted)
            {
                this.OnReadComplete(args, task, startTime);
                return false;
            }

            task.ContinueWith(static (t, s) =>
            {
                var (transport, args, startTime) = (Tuple<WebSocketTransport, TransportAsyncCallbackArgs, DateTime>) s;
                transport.OnReadComplete(args, t, startTime);
                args.CompletedCallback(args);
            }, Tuple.Create(this, args, startTime));
            return true;
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <returns>true if open is completed, otherwise false.</returns>
        protected override bool OpenInternal()
        {
            return true;
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>true if close is completed, otherwise false.</returns>
        protected override bool CloseInternal()
        {
            Task task = webSocket.CloseAsync(WebSocketCloseStatus.Empty, string.Empty, CancellationToken.None);
            if (task.IsCompleted)
            {
                if (task.IsFaulted)
                {
                    ExceptionDispatcher.Throw(task.Exception.InnerException);
                }
                else if (task.IsCanceled)
                {
                    throw new OperationCanceledException();
                }

                return true;
            }

            task.ContinueWith(static (t, s) =>
            {
                var thisPtr = (WebSocketTransport) s;
                var exception = t.IsFaulted ? t.Exception?.InnerException : t.IsCanceled ? new OperationCanceledException() : null;
                thisPtr.CompleteClose(false, exception);
            }, this);

            return false;
        }

        /// <summary>
        /// Aborts the object.
        /// </summary>
        protected override void AbortInternal()
        {
            this.webSocket.Dispose();
        }

        /// <summary>
        /// Sets a transport monitor for transport I/O operations.
        /// </summary>
        /// <param name="usageMeter">The transport monitor.</param>
        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.usageMeter = usageMeter;
        }

        internal static bool MatchScheme(string scheme)
        {
            return string.Equals(scheme, WebSocketTransportSettings.WebSockets, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(scheme, WebSocketTransportSettings.SecureWebSockets, StringComparison.OrdinalIgnoreCase);
        }

        void OnWriteComplete(TransportAsyncCallbackArgs args, Task t, ArraySegment<byte> buffer, ByteBuffer byteBuffer, DateTime startTime)
        {
            if (t.IsFaulted)
            {
                args.Exception = t.Exception?.InnerException;
            }
            else if (t.IsCanceled)
            {
                args.Exception = new OperationCanceledException();
            }
            else
            {
                args.BytesTransfered = buffer.Count;
                if (this.usageMeter != null)
                {
                    this.usageMeter.OnTransportWrite(0, buffer.Count, 0, DateTime.UtcNow.Subtract(startTime).Ticks);
                }
            }

            if (byteBuffer != null)
            {
                byteBuffer.Dispose();
            }
        }

        void OnReadComplete(TransportAsyncCallbackArgs args, Task<WebSocketReceiveResult> t, DateTime startTime)
        {
            if (t.IsFaulted)
            {
                args.Exception = t.Exception?.InnerException;
            }
            else if (t.IsCanceled)
            {
                args.Exception = new OperationCanceledException();
            }
            else
            {
                args.BytesTransfered = t.Result.Count;
                if (this.usageMeter != null)
                {
                    this.usageMeter.OnTransportRead(0, args.BytesTransfered, 0, DateTime.UtcNow.Subtract(startTime).Ticks);
                }
            }
        }
    }
}