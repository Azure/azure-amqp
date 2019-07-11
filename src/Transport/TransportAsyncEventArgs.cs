// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    
    /// <summary>
    /// Defines the callback arguments for asynchronous transport operations.
    /// </summary>
    public sealed class TransportAsyncCallbackArgs : IAsyncResult
    {
        /// <summary>
        /// Gets the buffer for transport I/O.
        /// </summary>
        public byte[] Buffer
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the offset of the buffer.
        /// </summary>
        public int Offset
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the count of bytes in the buffer.
        /// </summary>
        public int Count
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the <see cref="ByteBuffer"/> for the I/O.
        /// </summary>
        public ByteBuffer ByteBuffer
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a list of <see cref="ByteBuffer"/> for the I/O.
        /// </summary>
        public IList<ByteBuffer> ByteBufferList
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets or sets the completion callback.
        /// </summary>
        public Action<TransportAsyncCallbackArgs> CompletedCallback
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a transport.
        /// </summary>
        public TransportBase Transport
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a user token.
        /// </summary>
        public object UserToken
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a second user token.
        /// </summary>
        public object UserToken2
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a boolean value that indicates if the operation
        /// is completed synchronously.
        /// </summary>
        public bool CompletedSynchronously
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the number of bytes transfered.
        /// </summary>
        public int BytesTransfered
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the exception.
        /// </summary>
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

        /// <summary>
        /// Sets a buffer to be used for the I/O.
        /// </summary>
        /// <param name="buffer">The byte array.</param>
        /// <param name="offset">The offset in the array.</param>
        /// <param name="count">The count of bytes in the array.</param>
        public void SetBuffer(byte[] buffer, int offset, int count)
        {
            this.Buffer = buffer;
            this.Offset = offset;
            this.Count = count;
            this.ByteBuffer = null;
            this.ByteBufferList = null;
        }

        /// <summary>
        /// Sets a buffer for a write operation.
        /// </summary>
        /// <param name="byteBuffer">The write buffer.</param>
        public void SetWriteBuffer(ByteBuffer byteBuffer)
        {
            this.SetBuffer(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
            this.ByteBuffer = byteBuffer;
        }

        /// <summary>
        /// Sets a buffer for a read operation.
        /// </summary>
        /// <param name="byteBuffer">The read buffer.</param>
        public void SetReadBuffer(ByteBuffer byteBuffer)
        {
            this.SetBuffer(byteBuffer.Buffer, byteBuffer.WritePos, byteBuffer.Size);
            this.ByteBuffer = byteBuffer;
        }

        /// <summary>
        /// Sets a list of buffers to write.
        /// </summary>
        /// <param name="byteBufferList">The list of buffers.</param>
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

        /// <summary>
        /// Resets the arguments.
        /// </summary>
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
