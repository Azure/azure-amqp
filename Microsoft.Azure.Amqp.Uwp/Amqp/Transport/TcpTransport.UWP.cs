// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Runtime.InteropServices.WindowsRuntime;
    using Windows.Networking.Sockets;
    using Windows.Storage.Streams;

    sealed class TcpTransport : TransportBase
    {
        readonly StreamSocket socket;
        readonly TcpTransportSettings settings;
        ITransportMonitor monitor;
        OperationState writeState;
        OperationState readState;

        public TcpTransport(StreamSocket socket, TcpTransportSettings settings)
            : base("tcp")
        {
            this.socket = socket;
            this.settings = settings;
        }

        public StreamSocket Socket
        {
            get { return this.socket; }
        }

        public override string LocalEndPoint
        {
            get
            {
                return $"{this.socket.Information.LocalAddress.CanonicalName}:{this.socket.Information.LocalPort}";
            }
        }

        public override string RemoteEndPoint
        {
            get
            {
                return $"{this.settings.Host}:{this.settings.Port}";
            }
        }

        public override void SetMonitor(ITransportMonitor monitor)
        {
            this.monitor = monitor;
        }

        public sealed override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(this.writeState.Args == null, "Cannot write when a write is still in progress");
            IBuffer ibuffer;
            if (args.Buffer != null)
            {
                this.writeState.Args = args;
                ibuffer = args.Buffer.AsBuffer(args.Offset, args.Count);
            }
            else
            {
                Fx.Assert(args.ByteBufferList != null, "Buffer list should not be null when buffer is null");
                ArraySegment<byte> buffer;
                if (args.ByteBufferList.Count == 1)
                {
                    ByteBuffer byteBuffer = args.ByteBufferList[0];
                    buffer = new ArraySegment<byte>(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
                    this.writeState.Args = args;
                }
                else
                {
                    Fx.Assert(args.Count > 0, "args.Count should be set");
                    ByteBuffer temp = new ByteBuffer(args.Count, false, false);
                    for (int i = 0; i < args.ByteBufferList.Count; ++i)
                    {
                        ByteBuffer byteBuffer = args.ByteBufferList[i];
                        System.Buffer.BlockCopy(byteBuffer.Buffer, byteBuffer.Offset, temp.Buffer, temp.Length, byteBuffer.Length);
                        temp.Append(byteBuffer.Length);
                    }

                    buffer = new ArraySegment<byte>(temp.Buffer, 0, temp.Length);
                    this.writeState.Args = args;
                    this.writeState.Buffer = temp;
                }

                ibuffer = buffer.Array.AsBuffer(0, buffer.Count);
            }

            var t = this.socket.OutputStream.WriteAsync(ibuffer).AsTask();

            if (t.IsCompleted)
            {
                args.BytesTransfered = args.Count;
                args.CompletedSynchronously = true;
                this.writeState.Reset();
                if (this.writeState.Buffer != null)
                {
                    this.writeState.Buffer.Dispose();
                }

                return false;
            }

            t.ContinueWith(completion =>
            {
                var args2 = this.writeState.Args;
                if (completion.IsFaulted)
                {
                    args2.Exception = completion.Exception;
                }
                else if (completion.IsCanceled)
                {
                    args2.Exception = new OperationCanceledException();
                }
                else
                {
                    args2 = this.writeState.Args;
                    ByteBuffer buffer = this.writeState.Buffer;
                    this.writeState.Reset();

                    if (buffer != null)
                    {
                        buffer.Dispose();
                    }

                    Fx.Assert(args2.Count == completion.Result, "completion must have the same write count");
                    args2.BytesTransfered = args2.Count;
                }

                args2.CompletedSynchronously = false;

                Action<TransportAsyncCallbackArgs> callback = args2.CompletedCallback;
                if (callback != null)
                {
                    args2.CompletedCallback(args2);
                }
            });

            return true;
        }

        public sealed override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            // Read with buffer list not supported
            Fx.Assert(args.Buffer != null, "must have buffer to read");
            Fx.Assert(this.readState.Args == null, "Cannot read when a read is still in progress");
            this.readState.Args = args;

            var buffer = args.Buffer.AsBuffer(args.Offset, args.Count);
            var t = this.socket.InputStream.ReadAsync(buffer, (uint)args.Count, InputStreamOptions.Partial).AsTask();

            if (t.IsCompleted)
            {
                args.BytesTransfered = (int)t.Result.Length;
                args.CompletedSynchronously = true;
                this.readState.Reset();
                return false;
            }

            t.ContinueWith(completion =>
            {
                var args2 = this.readState.Args;
                if (completion.IsFaulted)
                {
                    args2.Exception = completion.Exception;
                }
                else if (completion.IsCanceled)
                {
                    args2.BytesTransfered = 0;
                }
                else
                {
                    this.readState.Reset();
                    args2.BytesTransfered = (int)completion.Result.Length;
                }

                args2.CompletedSynchronously = false;

                Action<TransportAsyncCallbackArgs> callback = args2.CompletedCallback;
                if (callback != null)
                {
                    args2.CompletedCallback(args2);
                }
            });

            return true;
        }

        protected override bool CloseInternal()
        {
            this.socket.Dispose();
            return true;
        }

        protected override void AbortInternal()
        {
            this.socket.Dispose();
        }

        struct OperationState
        {
            public TransportAsyncCallbackArgs Args;

            public ByteBuffer Buffer;

            public void Reset()
            {
                this.Args = null;
                this.Buffer = null;
            }
        }
    }
}
