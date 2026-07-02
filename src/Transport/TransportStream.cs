// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Encoding;

    // Highly-optimized Stream used only by TlsTransport.
    // * Sync Write appends into an in-memory buffer (no I/O). SslStream encrypts each
    //   source segment in place and hands the ciphertext to Write, which accumulates it.
    // * BeginFlushWrite issues a single async I/O of the accumulated ciphertext to the
    //   inner transport, avoiding both the pre-encryption merge copy and a second copy
    //   inside SslStream that would happen with one giant BeginWrite.
    // * BeginRead / BeginWrite (async) still go directly to the inner transport and
    //   are used by SslStream during the TLS handshake.
    sealed class TransportStream : Stream
    {
        static readonly Action<TransportAsyncCallbackArgs> onIOComplete = OnIOComplete;
        readonly TransportBase transport;
        ByteBuffer writeBuffer;

        public TransportStream(TransportBase transport)
        {
            this.transport = transport;
        }

        public override bool CanSeek
        {
            get { return false; }
        }

        public override bool CanRead
        {
            get { return true; }
        }

        public override bool CanWrite
        {
            get { return true; }
        }

        public override long Length
        {
            get { throw new InvalidOperationException(); }
        }

        public override long Position
        {
            get
            {
                throw new InvalidOperationException();
            }

            set
            {
                throw new InvalidOperationException();
            }
        }

        public override void Flush()
        {
            // No-op. TlsTransport is responsible for calling BeginFlushWrite after a
            // batch of sync Writes. SslStream does not call Flush between Write calls
            // for us to worry about mid-batch flushing.
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            // TransportAsyncCallbackArgs only supports AsyncCallback. EndRead does not block
            // until the operation is completed. So need an event here. The sync Read method
            // is called in mono environment.
            using (var doneEvent = new ManualResetEventSlim())
            {
                var asyncResult = this.BeginRead(buffer, offset, count, static ar => ((ManualResetEventSlim)ar.AsyncState).Set(), doneEvent);
                doneEvent.Wait();
                return this.EndRead(asyncResult);
            }
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            if (count == 0)
            {
                return;
            }

            // Buffer the ciphertext produced by SslStream. It will be flushed to the
            // inner transport as a single I/O by BeginFlushWrite.
            if (this.writeBuffer == null)
            {
                this.writeBuffer = new ByteBuffer(count, true);
            }

            AmqpBitConverter.WriteBytes(this.writeBuffer, buffer, offset, count);
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new InvalidOperationException();
        }

        public override void SetLength(long value)
        {
            throw new InvalidOperationException();
        }

        public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (p, k, c, s) => ((TransportStream)s).BeginWrite(p.Array, p.Offset, p.Count, c, s),
                static (a) => ((TransportStream)a.AsyncState).EndWrite(a),
                new ArraySegment<byte>(buffer, offset, count),
                cancellationToken,
                this);
        }

        // Sends the accumulated ciphertext (from prior sync Write calls) to the inner
        // transport as a single async I/O. Must not be called if Write was never called.
        public IAsyncResult BeginFlushWrite(AsyncCallback callback, object state)
        {
            if (this.writeBuffer == null)
            {
                throw new InvalidOperationException("Write must be called before flushing");
            }

            try
            {
                return this.BeginWrite(this.writeBuffer.Buffer, this.writeBuffer.Offset, this.writeBuffer.Length, callback, state);
            }
            catch
            {
                this.writeBuffer.Dispose();
                this.writeBuffer = null;
                throw;
            }
        }

        public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
        {
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.SetBuffer(buffer, offset, count);
            args.CompletedCallback = onIOComplete;
            args.UserToken = this;
            args.UserToken2 = Tuple.Create(callback, state);
            if (!this.transport.WriteAsync(args))
            {
                Fx.Assert(args.CompletedSynchronously, "args.CompletedSynchronously should be true if not pending");
                this.CompleteOperation(args);
            }
            return args;
        }

        public override void EndWrite(IAsyncResult asyncResult)
        {
            var args = (TransportAsyncCallbackArgs)asyncResult;
            if (this.writeBuffer != null)
            {
                Fx.Assert(args.Buffer == this.writeBuffer.Buffer, "Wrong write buffer");
                this.writeBuffer.Dispose();
                this.writeBuffer = null;
            }

            if (args.Exception != null)
            {
                throw args.Exception;
            }
        }

        public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            if (count == 0)
            {
                return Task.FromResult(0);
            }

            return Task.Factory.FromAsync(
                static (p, k, c, s) => ((TransportStream)s).BeginRead(p.Array, p.Offset, p.Count, c, s),
                static (a) => ((TransportStream)a.AsyncState).EndRead(a),
                new ArraySegment<byte>(buffer, offset, count),
                cancellationToken,
                this);
        }

        public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
        {
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.SetBuffer(buffer, offset, count);
            args.CompletedCallback = onIOComplete;
            args.UserToken = this;
            args.UserToken2 = Tuple.Create(callback, state);
            if (!this.transport.ReadAsync(args))
            {
                Fx.Assert(args.CompletedSynchronously, "args.CompletedSynchronously should be true if not pending");
                this.CompleteOperation(args);
            }

            return args;
        }

        public override int EndRead(IAsyncResult asyncResult)
        {
            var args = (TransportAsyncCallbackArgs)asyncResult;
            if (args.Exception != null)
            {
                throw args.Exception;
            }

            return args.BytesTransfered;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.transport.SafeClose();
            }
        }

        static void OnIOComplete(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(!args.CompletedSynchronously, "args.CompletedSynchronously should be false from async callback");
            TransportStream thisPtr = (TransportStream)args.UserToken;
            thisPtr.CompleteOperation(args);
        }

        void CompleteOperation(TransportAsyncCallbackArgs args)
        {
            var userState = (Tuple<AsyncCallback, object>)args.UserToken2;
            AsyncCallback callback = userState.Item1;
            object state = userState.Item2;
            args.UserToken = state;
            callback?.Invoke(args);
        }
    }
}
