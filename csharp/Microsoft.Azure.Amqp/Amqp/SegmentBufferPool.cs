// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    
    sealed class SegmentBufferPool
    {
        readonly int segmentSize;
        readonly byte[] heap;
        readonly int[] offsets;
        readonly object lockObject = new object();
        int top;

        public SegmentBufferPool(int segmentSize, int count)
        {
            this.segmentSize = segmentSize;
            this.heap = new byte[segmentSize * count];
            this.offsets = new int[count];
            this.top = count - 1;
            for (int i = count - 1, offset = 0; i >= 0; --i, offset += segmentSize)
            {
                this.offsets[i] = offset;
            }
        }

        public int SegmentSize
        {
            get { return this.segmentSize; }
        }

        public ArraySegment<byte> TakeBuffer(int bufferSize)
        {
            if (bufferSize > this.segmentSize)
            {
                throw new ArgumentOutOfRangeException("bufferSize");
            }

            // TODO: make this lock free (use Interlocked.Increment/Decrement/CompareExchange)
            int offset;
            lock (this.lockObject)
            {
                if (this.top >= 0)
                {
                    offset = this.offsets[this.top];
                    --this.top;
                }
                else
                {
                    offset = -1;
                }
            }

            if (offset >= 0)
            {
                return new ArraySegment<byte>(this.heap, offset, bufferSize);
            }
            else
            {
                return new ArraySegment<byte>(new byte[bufferSize]);
            }
        }

        public void ReturnBuffer(ArraySegment<byte> buffer)
        {
            if (buffer.Array == this.heap)
            {
                Fx.Assert(buffer.Offset >= 0 && buffer.Offset + buffer.Count <= this.heap.Length, "out of range");
                Fx.Assert(buffer.Offset % this.segmentSize == 0, "invalid offset");
                Fx.Assert(buffer.Count <= this.segmentSize, "invalid size");

                lock (this.lockObject)
                {
                    if (this.top < this.offsets.Length - 1)
                    {
                        ++this.top;
                        this.offsets[this.top] = buffer.Offset;
                    }
                }
            }
        }
    }
}
