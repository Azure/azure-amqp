// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#if WINDOWS_UWP
namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Security.Authentication;
    using System.Security.Principal;
    using System.Runtime.InteropServices.WindowsRuntime;
    using Windows.Networking.Sockets;
    using System.Threading.Tasks;

    sealed class TlsTransport : TransportBase, IDisposable
    {
        const SslProtocols DefaultSslProtocols = SslProtocols.Tls | SslProtocols.Ssl3; // SslProtocols.Default from .NET 4.5
        readonly TransportBase innerTransport;
        StreamSocket socket;

        TlsTransportSettings tlsSettings;
        OperationState writeState;
        OperationState readState;

        public TlsTransport(TransportBase innerTransport, TlsTransportSettings tlsSettings)
            : base("tls", innerTransport.Identifier)
        {
            this.innerTransport = innerTransport;
            this.tlsSettings = tlsSettings;

            var tcpTransport = innerTransport as TcpTransport;
            if (tcpTransport != null)
            {
                this.socket = tcpTransport.Socket;
            }
            else
            {
                throw new NotSupportedException("Only TCP transport is supported");
            }
        }

        public override EndPoint LocalEndPoint
        {
            get { return this.innerTransport.LocalEndPoint; }
        }

        public override EndPoint RemoteEndPoint
        {
            get { return this.innerTransport.RemoteEndPoint; }
        }

        public override bool IsSecure
        {
            get { return true; }
        }

        public override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(this.writeState.Args == null, "Cannot write when a write is still in progress");
            Windows.Storage.Streams.IBuffer ibuffer;
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
                    // Copy all buffers into one big buffer to avoid SSL overhead
                    Fx.Assert(args.Count > 0, "args.Count should be set");
                    ByteBuffer temp = new ByteBuffer(args.Count, false, false);
                    for (int i = 0; i < args.ByteBufferList.Count; ++i)
                    {
                        ByteBuffer byteBuffer = args.ByteBufferList[i];
                        Buffer.BlockCopy(byteBuffer.Buffer, byteBuffer.Offset, temp.Buffer, temp.Length, byteBuffer.Length);
                        temp.Append(byteBuffer.Length);
                    }

                    buffer = new ArraySegment<byte>(temp.Buffer, 0, temp.Length);
                    this.writeState.Args = args;
                    this.writeState.Buffer = temp;
                }

                ibuffer = buffer.Array.AsBuffer(0, buffer.Count);
            }

            var t = this.socket.OutputStream.WriteAsync(ibuffer).AsTask();
            t.ContinueWith(completion =>
            {
                var args2 = this.readState.Args;
                if (completion.IsFaulted)
                {
                    if (Fx.IsFatal(completion.Exception))
                    {
                        throw completion.Exception;
                    }
                    args2.Exception = completion.Exception;
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

        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            // Read with buffer list not supported
            Fx.Assert(args.Buffer != null, "must have buffer to read");
            Fx.Assert(this.readState.Args == null, "Cannot read when a read is still in progress");
            this.readState.Args = args;

            var buffer = args.Buffer.AsBuffer(args.Offset, args.Count);
            var t = this.socket.InputStream.ReadAsync(buffer, (uint)args.Count, Windows.Storage.Streams.InputStreamOptions.Partial).AsTask();
            t.ContinueWith(completion =>
            {
                var args2 = this.readState.Args;
                if (completion.IsFaulted)
                {
                    if (Fx.IsFatal(completion.Exception))
                    {
                        throw completion.Exception;
                    }
                    args2.Exception = completion.Exception;
                }
                else
                {
                    this.readState.Reset();

                    Fx.Assert(args2.Count == completion.Result.Length, "completion must have the same write count");
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

        protected override bool OpenInternal()
        {
            var t = this.socket.UpgradeToSslAsync(SocketProtectionLevel.Tls12, new Windows.Networking.HostName(this.tlsSettings.TargetHost)).AsTask();
            t.ContinueWith(completion =>
            {
                if (completion.IsFaulted)
                {
                    if (Fx.IsFatal(completion.Exception))
                    {
                        throw completion.Exception;
                    }
                }

                this.CompleteOpen(false, completion.Exception);
            });
            return false;
        }

        protected override bool CloseInternal()
        {
            if (this.socket != null)
            {
                this.socket.Dispose();
            }
            return true;
        }

        protected override void AbortInternal()
        {
            this.innerTransport.Abort();
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            if (this.socket != null)
            {
                this.socket.Dispose();
            }
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
#endif
