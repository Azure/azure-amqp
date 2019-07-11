// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Provides a wrapper to read bytes from and write bytes to a byte array.
    /// It also pools the byte array to reduce allocation.
    /// </summary>
    public class ByteBuffer : IDisposable, IAmqpSerializable
    {
        static readonly InternalBufferManager PooledBufferManager = InternalBufferManager.CreatePooledBufferManager(
            50 * 1024 * 1024, int.MaxValue);

        static InternalBufferManager TransportBufferManager;
        static readonly object syncRoot = new object();

        //
        // A buffer has start and end and two cursors (read/write)
        // All four are absolute values.
        //
        //   +---------+--------------+----------------+
        // start      read          write             end
        //
        // read - start: already consumed
        // write - read: Length (bytes to be consumed)
        // end - write: Size (free space to write)
        // end - start: Capacity
        //
        byte[] buffer;
        int start;
        int read;
        int write;
        int end;
        bool autoGrow;
        int references;
        InternalBufferManager bufferManager;
        ByteBuffer innerBuffer;

        /// <summary>
        /// Initializes the pool of buffers used for transport read and write.
        /// </summary>
        /// <param name="bufferSize">The buffer size in bytes.</param>
        /// <param name="maxCount">Number of buffers to allocate.</param>
        /// <remarks>This method allocates the buffers. It should
        /// be called as early as possible when the process starts, so that
        /// when a buffer is pinned for pending I/O, it doesn't cause significant
        /// heap fragmentation.</remarks>
        public static void InitTransportBufferManager(int bufferSize, int maxCount)
        {
            if (TransportBufferManager == null)
            {
                lock (syncRoot)
                {
                    if (TransportBufferManager == null)
                    {
                        TransportBufferManager = InternalBufferManager.CreatePreallocatedBufferManager(bufferSize, maxCount);
                    }
                }
            }
        }

        /// <summary>
        /// Initializes the buffer from a byte array.
        /// </summary>
        /// <param name="buffer">The byte array.</param>
        public ByteBuffer(byte[] buffer)
            : this(buffer, 0, 0, buffer.Length, false, null)
        {
        }

        /// <summary>
        /// Initializes the buffer from a byte array.
        /// </summary>
        /// <param name="buffer">The byte array.</param>
        /// <param name="autoGrow">true for allocating a bigger buffer when the
        /// current one does not have space for write.</param>
        public ByteBuffer(byte[] buffer, bool autoGrow)
            : this(buffer, 0, 0, buffer.Length, autoGrow, null)
        {
        }

        /// <summary>
        /// Initializes the buffer from a byte array segment.
        /// </summary>
        /// <param name="array">The byte array segment.</param>
        public ByteBuffer(ArraySegment<byte> array)
            : this(array.Array, array.Offset, array.Count, array.Count, false, null)
        {
        }

        /// <summary>
        /// Initializes the buffer for a given size.
        /// </summary>
        /// <param name="size">The required size in bytes.</param>
        /// <param name="autoGrow">true for allocating a bigger buffer when the
        /// current one does not have space for write.</param>
        public ByteBuffer(int size, bool autoGrow)
            : this(size, autoGrow, false)
        {
        }

        /// <summary>
        /// Initializes the buffer for a given size.
        /// </summary>
        /// <param name="size">The required size in bytes.</param>
        /// <param name="autoGrow">true for allocating a bigger buffer when the
        /// current one does not have space for write.</param>
        /// <param name="isTransportBuffer">true for allocating a byte array
        /// from the transport buffer pool.</param>
        public ByteBuffer(int size, bool autoGrow, bool isTransportBuffer)
            : this(ByteBuffer.AllocateBufferFromPool(size, isTransportBuffer), autoGrow, size)
        {
        }

        /// <summary>
        /// Initializes the buffer from a byte array segment.
        /// </summary>
        /// <param name="buffer">The byte array.</param>
        /// <param name="offset">The offset.</param>
        /// <param name="count">Number of bytes from the offset.</param>
        public ByteBuffer(byte[] buffer, int offset, int count)
            : this(buffer, offset, count, count, false, null)
        {
        }

        // the constructor for serializer
        ByteBuffer()
        {
        }

        ByteBuffer(ManagedBuffer bufferReference, bool autoGrow, int size)
            : this(bufferReference.Buffer, 0, 0, size, autoGrow, bufferReference.BufferManager)
        {
        }

        ByteBuffer(byte[] buffer, int offset, int count, int size, bool autoGrow, InternalBufferManager bufferManager)
        {
            Fx.Assert(buffer != null, "buffer cannot be null");
            this.buffer = buffer;
            this.start = offset;
            this.read = offset;
            this.write = offset + count;
            this.end = offset + size;
            this.autoGrow = autoGrow;
            this.bufferManager = bufferManager;
            this.references = 1;
#if DEBUG
            if (AmqpTrace.AmqpDebug && bufferManager != null)
            {
                this.stack = new StackTrace();
            }
#endif
        }

#if DEBUG
        StackTrace stack;

        /// <summary>
        /// Finalizer for the object.
        /// </summary>
        ~ByteBuffer()
        {
            if (this.stack == null)
            {
                System.Diagnostics.Trace.WriteLine(string.Format("{0} leak {1} bytes from {2}",
                    AppDomain.CurrentDomain.FriendlyName, this.end, this.stack));
            }
        }
#endif

        static ManagedBuffer AllocateBufferFromPool(int size, bool isTransportBuffer)
        {
            return AllocateBuffer(size, isTransportBuffer ? TransportBufferManager : PooledBufferManager);
        }

        // attempts to allocate using the supplied buffer manager, falls back to the default buffer manager on failure
        static ManagedBuffer AllocateBuffer(int size, InternalBufferManager bufferManager)
        {
            if (bufferManager != null)
            {
                byte[] buffer = bufferManager.TakeBuffer(size);
                if (buffer != null)
                {
                    return new ManagedBuffer(buffer, bufferManager);
                }
            }

            return new ManagedBuffer(PooledBufferManager.TakeBuffer(size), PooledBufferManager);
        }

        /// <summary>
        /// Gets the byte array.
        /// </summary>
        public byte[] Buffer
        {
            get { return this.buffer; }
        }

        /// <summary>
        /// Gets the capacity of the buffer.
        /// </summary>
        public int Capacity
        {
            get { return this.end - this.start; }
        }

        /// <summary>
        /// Gets the current read offset.
        /// </summary>
        public int Offset
        {
            get { return this.read; }
        }

        /// <summary>
        /// Gets the remaining size for write.
        /// </summary>
        public int Size
        {
            get { return this.end - this.write; }
        }

        /// <summary>
        /// Gets the remainng bytes for read.
        /// </summary>
        public int Length
        {
            get { return this.write - this.read; }
        }

        /// <summary>
        /// Gets the current write offset.
        /// </summary>
        public int WritePos
        {
            get { return this.write; }
        }

        /// <summary>
        /// Validates if the buffer has sufficient bytes for read or space for write.
        /// </summary>
        /// <param name="write">true if validation is for writing bytes.</param>
        /// <param name="dataSize">The requested size.</param>
        public void Validate(bool write, int dataSize)
        {
            bool valid = false;
            if (write)
            {
                if (this.Size < dataSize && this.autoGrow)
                {
                    if (this.references != 1)
                    {
                        throw new InvalidOperationException("Cannot grow the current buffer because it has more than one references");
                    }

                    int newSize = Math.Max(this.Capacity * 2, this.Capacity + dataSize);
                    ManagedBuffer newBuffer;
                    if (this.bufferManager != null)
                    {
                        newBuffer = ByteBuffer.AllocateBuffer(newSize, this.bufferManager);
                    }
                    else
                    {
                        newBuffer = new ManagedBuffer(new byte[newSize], null);
                    }
                    
                    System.Buffer.BlockCopy(this.buffer, this.start, newBuffer.Buffer, 0, this.Capacity);

                    int consumed = this.read - this.start;
                    int written = this.write - this.start;

                    this.start = 0;
                    this.read = consumed;
                    this.write = written;
                    this.end = newSize;

                    if (this.bufferManager != null)
                    {
                        this.bufferManager.ReturnBuffer(this.buffer);
                    }
                    this.buffer = newBuffer.Buffer;
                    this.bufferManager = newBuffer.BufferManager;
                }

                valid = this.Size >= dataSize;
            }
            else
            {
                valid = this.Length >= dataSize;
            }

            if (!valid)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, AmqpResources.GetString(AmqpResources.AmqpInsufficientBufferSize, dataSize, write ? this.Size : this.Length));
            }
        }

        /// <summary>
        /// Advances the read end position.
        /// </summary>
        /// <param name="size"></param>
        public void Append(int size)
        {
            Fx.Assert(size >= 0, "size must be positive.");
            Fx.Assert((this.write + size) <= this.end, "Append size too large.");
            this.write += size;
        }

        /// <summary>
        /// Advances the read cursor.
        /// </summary>
        /// <param name="size"></param>
        public void Complete(int size)
        {
            Fx.Assert(size >= 0, "size must be positive.");
            Fx.Assert((this.read + size) <= this.write, "Complete size too large.");
            this.read += size;
        }

        /// <summary>
        /// Sets the read cursor.
        /// </summary>
        /// <param name="seekPosition">The absolute position in the byte array.</param>
        public void Seek(int seekPosition)
        {
            Fx.Assert(seekPosition >= this.start, "seekPosition must not be before start.");
            Fx.Assert(seekPosition <= this.write, "seekPosition must not be after write.");
            this.read = seekPosition;
        }

        /// <summary>
        /// Resets the read and write cursor.
        /// </summary>
        public void Reset()
        {
            this.read = this.start;
            this.write = this.start;
        }

        /// <summary>
        /// Gets a slice of the buffer.
        /// </summary>
        /// <param name="position">The start position.</param>
        /// <param name="length">Number of bytes.</param>
        /// <returns></returns>
        public ByteBuffer GetSlice(int position, int length)
        {
            if (this.innerBuffer != null)
            {
                return this.innerBuffer.GetSlice(position, length);
            }

#if DEBUG
            if (AmqpTrace.AmqpDebug && this.bufferManager != null)
            {
                this.stack = new StackTrace();
            }
#endif
            ByteBuffer wrapped = this.AddReference();
            return new ByteBuffer(wrapped.buffer, position, length, length, false, null)
            {
                innerBuffer = wrapped
            };
        }

        /// <summary>
        /// Adjusts the read and write cursor.
        /// </summary>
        /// <param name="offset">The read cursor.</param>
        /// <param name="length">The number of bytes for read. The write cursor
        /// will be after the length.</param>
        public void AdjustPosition(int offset, int length)
        {
            Fx.Assert(offset >= this.start, "Invalid offset!");
            Fx.Assert(offset + length <= this.end, "length too large!");
            this.read = offset;
            this.write = this.read + length;
        }

        /// <summary>
        /// Releases the reference on the buffer. When the reference is 0, the
        /// byte array is returned.
        /// </summary>
        public void Dispose()
        {
            this.RemoveReference();
        }

        /// <summary>
        /// Tries to add a reference on the buffer.
        /// </summary>
        /// <returns>true on success, false if the buffer is already disposed.</returns>
        public bool TryAddReference()
        {
            if (Interlocked.Increment(ref this.references) <= 1)
            {
                Interlocked.Decrement(ref this.references);
                return false;
            }

            if (this.innerBuffer != null && !this.innerBuffer.TryAddReference())
            {
                Interlocked.Decrement(ref this.references);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Adds a reference on the buffer. Throws <see cref="InvalidOperationException"/>
        /// if the buffer is already disposed.
        /// </summary>
        /// <returns>The current buffer.</returns>
        public ByteBuffer AddReference()
        {
            if (!this.TryAddReference())
            {
                throw new InvalidOperationException(AmqpResources.AmqpBufferAlreadyReclaimed);
            }

            return this;
        }

        /// <summary>
        /// Removes a reference on the buffer. When the reference is 0, the
        /// byte array is returned.
        /// </summary>
        public void RemoveReference()
        {
            int refCount = Interlocked.Decrement(ref this.references);
#if DEBUG
            if (refCount <= 0)
            {
                this.stack = null;
            }
#endif

            if (this.innerBuffer != null)
            {
                this.innerBuffer.RemoveReference();
            }
            else if (refCount == 0)
            {
                byte[] bufferToRelease = this.buffer;
                this.buffer = null;
                if (bufferToRelease != null && this.bufferManager != null)
                {
                    this.bufferManager.ReturnBuffer(bufferToRelease);
                }
            }
        }

        [Conditional("DEBUG")]
        internal void AssertReferences(int value)
        {
            if (this.innerBuffer != null)
            {
                this.innerBuffer.AssertReferences(value);
            }
            else
            {
                Fx.Assert(this.references == value, string.Format("Expected {0} Actual {1}", value, this.references));
            }
        }

        int IAmqpSerializable.EncodeSize
        {
            get { return BinaryEncoding.GetEncodeSize(new ArraySegment<byte>(this.buffer, this.Offset, this.Length)); }
        }

        void IAmqpSerializable.Encode(ByteBuffer buffer)
        {
            BinaryEncoding.Encode(new ArraySegment<byte>(this.buffer, this.Offset, this.Length), buffer);
        }

        void IAmqpSerializable.Decode(ByteBuffer buffer)
        {
            ArraySegment<byte> segment = BinaryEncoding.Decode(buffer, 0, false);

            this.innerBuffer = buffer.AddReference();
            this.references = 1;
            this.buffer = segment.Array;
            this.start = this.read = segment.Offset;
            this.write = this.end = segment.Offset + segment.Count;
        }

        struct ManagedBuffer
        {
            public readonly InternalBufferManager BufferManager;

            public readonly byte[] Buffer;

            public ManagedBuffer(byte[] buffer, InternalBufferManager bufferManager)
            {
                this.Buffer = buffer;
                this.BufferManager = bufferManager;
            }
        }
    }
}
