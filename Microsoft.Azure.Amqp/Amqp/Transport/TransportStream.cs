// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    sealed class TransportStream : Stream
    {
        static readonly Action<TransportAsyncCallbackArgs> onIOComplete = OnIOComplete;
        readonly TransportBase transport;

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
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            // TransportAsyncCallbackArgs only supports AsyncCallback. EndRead does not block
            // until the operation is completed. So need an event here. The sync Read method
            // is called in mono environment.
            using (var doneEvent = new ManualResetEventSlim())
            {
                var asyncResult = this.BeginRead(buffer, offset, count, ar => ((ManualResetEventSlim)ar.AsyncState).Set(), doneEvent);
                doneEvent.Wait();
                return this.EndRead(asyncResult);
            }
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            // This should not be called but implement it anyway
            using (var doneEvent = new ManualResetEventSlim())
            {
                var asyncResult = this.BeginWrite(buffer, offset, count, ar => ((ManualResetEventSlim)ar.AsyncState).Set(), doneEvent);
                doneEvent.Wait();
                this.EndWrite(asyncResult);
            }
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
                (thisPtr, a, c, s) => thisPtr.BeginWrite(a.Array, a.Offset, a.Count, c, s),
                (a) => ((TransportStream)a.AsyncState).EndWrite(a),
                this,
                new ArraySegment<byte>(buffer, offset, count),
                this);
        }

#if NETSTANDARD1_3
        IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
#else
        public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
#endif
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

#if NETSTANDARD1_3
        void EndWrite(IAsyncResult asyncResult)
#else
        public override void EndWrite(IAsyncResult asyncResult)
#endif
        {
            var args = (TransportAsyncCallbackArgs)asyncResult;
            if (args.Exception != null)
            {
                args.Exception.Rethrow();
            }
        }

        public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            if (count == 0)
            {
                return Task.FromResult(0);
            }

            return Task.Factory.FromAsync(
                (thisPtr, a, c, s) => ((TransportStream)s).BeginRead(a.Array, a.Offset, a.Count, c, s),
                (a) => ((TransportStream)a.AsyncState).EndRead(a),
                this,
                new ArraySegment<byte>(buffer, offset, count),
                this);
        }

#if NETSTANDARD1_3
        IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
#else
        public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
#endif
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

#if NETSTANDARD1_3
        int EndRead(IAsyncResult asyncResult)
#else
        public override int EndRead(IAsyncResult asyncResult)
#endif
        {
            var args = (TransportAsyncCallbackArgs)asyncResult;
            if (args.Exception != null)
            {
                args.Exception.Rethrow();
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
