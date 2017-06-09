// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;

    // This class is not thread safe
    public sealed class ByteBuffer : IDisposable, ICloneable, IAmqpSerializable
    {
        static readonly InternalBufferManager BufferManager = InternalBufferManager.Create(50 * 1024 * 1024, int.MaxValue, false);

        static InternalBufferManager TransportBufferManager;
        static object syncRoot = new object();

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

        public static void InitBufferManagers()
        {
            if (TransportBufferManager == null)
            {
                lock (syncRoot)
                {
                    if (TransportBufferManager == null)
                    {
                        TransportBufferManager = InternalBufferManager.Create(48 * 1024 * 1024, AmqpConstants.TransportBufferSize, true);
                    }
                }
            }
        }

        public ByteBuffer(byte[] buffer)
            : this(buffer, 0, 0, buffer.Length, false, null)
        {
        }

        public ByteBuffer(byte[] buffer, bool autoGrow)
            : this(buffer, 0, 0, buffer.Length, autoGrow, null)
        {
        }

        public ByteBuffer(ArraySegment<byte> array)
            : this(array.Array, array.Offset, array.Count, array.Count, false, null)
        {
        }

        public ByteBuffer(int size, bool autoGrow)
            : this(size, autoGrow, false)
        {
        }

        public ByteBuffer(int size, bool autoGrow, bool isTransportBuffer)
            : this(ByteBuffer.AllocateBufferFromPool(size, isTransportBuffer), autoGrow, size)
        {
        }

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
        }

        static ManagedBuffer AllocateBufferFromPool(int size, bool isTransportBuffer)
        {
            return AllocateBuffer(size, isTransportBuffer ? TransportBufferManager : BufferManager);
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

            return new ManagedBuffer(BufferManager.TakeBuffer(size), BufferManager);
        }

        public byte[] Buffer
        {
            get { return this.buffer; }
        }

        public int Capacity
        {
            get { return this.end - this.start; }
        }

        public int Offset
        {
            get { return this.read; }
        }

        public int Size
        {
            get { return this.end - this.write; }
        }

        public int Length
        {
            get { return this.write - this.read; }
        }

        public int WritePos
        {
            get { return this.write; }
        }

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

        public void Append(int size)
        {
            Fx.Assert(size >= 0, "size must be positive.");
            Fx.Assert((this.write + size) <= this.end, "Append size too large.");
            this.write += size;
        }

        public void Complete(int size)
        {
            Fx.Assert(size >= 0, "size must be positive.");
            Fx.Assert((this.read + size) <= this.write, "Complete size too large.");
            this.read += size;
        }

        public void Seek(int seekPosition)
        {
            Fx.Assert(seekPosition >= this.start, "seekPosition must not be before start.");
            Fx.Assert(seekPosition <= this.write, "seekPosition must not be after write.");
            this.read = seekPosition;
        }

        public void Reset()
        {
            this.read = this.start;
            this.write = this.start;
        }

        public object Clone()
        {
            this.AddReference();
            return this;
        }

        public ByteBuffer GetSlice(int position, int length)
        {
            ByteBuffer wrapped = this.AddReference();
            return new ByteBuffer(wrapped.buffer, position, length, length, false, null)
            {
                innerBuffer = wrapped
            };
        }

        public void AdjustPosition(int offset, int length)
        {
            Fx.Assert(offset >= this.start, "Invalid offset!");
            Fx.Assert(offset + length <= this.end, "length too large!");
            this.read = offset;
            this.write = this.read + length;
        }

        public void Dispose()
        {
            this.RemoveReference();
        }

        internal bool TryAddReference()
        {
            if (Interlocked.Increment(ref this.references) == 1)
            {
                Interlocked.Decrement(ref this.references);
                return false;
            }

            return true;
        }

        internal ByteBuffer AddReference()
        {
            if (!this.TryAddReference())
            {
                throw new InvalidOperationException(AmqpResources.AmqpBufferAlreadyReclaimed);
            }

            return this;
        }

        internal void RemoveReference()
        {
            if (this.references > 0)
            {
                if (Interlocked.Decrement(ref this.references) == 0)
                {
                    byte[] bufferToRelease = this.buffer;
                    this.buffer = null;
                    if (this.bufferManager != null)
                    {
                        this.bufferManager.ReturnBuffer(bufferToRelease);
                    }
                    else if (this.innerBuffer != null)
                    {
                        this.innerBuffer.Dispose();
                    }
                }
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
            public InternalBufferManager BufferManager;

            public byte[] Buffer;

            public ManagedBuffer(byte[] buffer, InternalBufferManager bufferManager)
            {
                this.Buffer = buffer;
                this.BufferManager = bufferManager;
            }
        }
    }
}
