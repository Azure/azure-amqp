// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    
    public sealed class TransportAsyncCallbackArgs : IAsyncResult
    {
        public byte[] Buffer
        {
            get;
            private set;
        }

        public int Offset
        {
            get;
            private set;
        }

        public int Count
        {
            get;
            private set;
        }

        public ByteBuffer ByteBuffer
        {
            get;
            private set;
        }

        public IList<ByteBuffer> ByteBufferList
        {
            get;
            private set;
        }

        public Action<TransportAsyncCallbackArgs> CompletedCallback
        {
            get;
            set;
        }

        public TransportBase Transport
        {
            get;
            set;
        }

        public object UserToken
        {
            get;
            set;
        }

        public object UserToken2
        {
            get;
            set;
        }

        public bool CompletedSynchronously
        {
            get;
            set;
        }

        public int BytesTransfered
        {
            get;
            set;
        }

        public Exception Exception
        {
            get;
            set;
        }

        object IAsyncResult.AsyncState
        {
            get { return this.UserToken; }
        }

        WaitHandle IAsyncResult.AsyncWaitHandle
        {
            get { throw new NotSupportedException(); }
        }

        bool IAsyncResult.CompletedSynchronously
        {
            get { return this.CompletedSynchronously; }
        }

        bool IAsyncResult.IsCompleted
        {
            get { return this.BytesTransfered == this.Count; }
        }

        public void SetBuffer(byte[] buffer, int offset, int count)
        {
            this.Buffer = buffer;
            this.Offset = offset;
            this.Count = count;
            this.ByteBuffer = null;
            this.ByteBufferList = null;
        }

        public void SetBuffer(ByteBuffer byteBuffer)
        {
            this.SetBuffer(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
            this.ByteBuffer = byteBuffer;
        }

        public void SetBuffer(IList<ByteBuffer> byteBufferList)
        {
            Fx.Assert(this.ByteBufferList == null, "A previous buffer is still pending.");
            this.SetBuffer(null, 0, 0);
            this.ByteBufferList = byteBufferList;
            this.Count = 0;
            foreach(ByteBuffer byteBuffer in byteBufferList)
            {
                this.Count += byteBuffer.Length;
            }
        }

        public void Reset()
        {
            if (this.ByteBuffer != null)
            {
                this.ByteBuffer.Dispose();
            }
            else if(this.ByteBufferList != null)
            {
                foreach(ByteBuffer byteBuffer in this.ByteBufferList)
                {
                    byteBuffer.Dispose();
                }
            }

            this.SetBuffer(null, 0, 0);
            this.UserToken = null;
            this.BytesTransfered = 0;
            this.Exception = null;
        }
    }
}
