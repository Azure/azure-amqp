// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    sealed class SegmentBufferPool
    {
        readonly int segmentSize;
        readonly int maxCount;
        readonly object lockObject;
        byte[] heap;
        int[] offsets;
        int top;

        public SegmentBufferPool(int segmentSize, int maxCount)
        {
            this.segmentSize = segmentSize;
            this.maxCount = maxCount;
            this.lockObject = new object();
            this.Initialize(4);
        }

        public int SegmentSize
        {
            get { return this.segmentSize; }
        }

        public ArraySegment<byte> TakeBuffer(int bufferSize)
        {
            if (bufferSize > this.segmentSize)
            {
                throw new ArgumentOutOfRangeException(nameof(bufferSize));
            }

            lock (this.lockObject)
            {
                if (this.top >= 0)
                {
                    return new ArraySegment<byte>(this.heap, this.offsets[this.top--], bufferSize);
                }
                else if (this.offsets.Length < this.maxCount)
                {
                    this.Initialize(Math.Min(this.offsets.Length * 2, this.maxCount));
                    return new ArraySegment<byte>(this.heap, this.offsets[this.top--], bufferSize);
                }
            }

            return new ArraySegment<byte>(new byte[bufferSize]);
        }

        public void ReturnBuffer(ArraySegment<byte> buffer)
        {
            Fx.Assert(buffer.Offset % this.segmentSize == 0, "invalid offset");
            Fx.Assert(buffer.Count <= this.segmentSize, "invalid size");

            lock (this.lockObject)
            {
                if (buffer.Array == this.heap && this.top < this.offsets.Length - 1)
                {
                    this.offsets[++this.top] = buffer.Offset;
                }
            }
        }

        void Initialize(int count)
        {
            this.heap = new byte[segmentSize * count];
            this.offsets = new int[count];
            this.top = count - 1;
            for (int i = count - 1, offset = 0; i >= 0; --i, offset += segmentSize)
            {
                this.offsets[i] = offset;
            }
        }
    }
}
