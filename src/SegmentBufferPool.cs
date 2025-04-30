// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Buffers;

    sealed class SegmentBufferPool
    {
        readonly ArrayPool<byte> arrayPool;
        readonly int segmentSize;

        public SegmentBufferPool(int segmentSize, int maxCount)
        {
            this.segmentSize = segmentSize;
            arrayPool = ArrayPool<byte>.Create(segmentSize, maxCount);
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

            return new ArraySegment<byte>(this.arrayPool.Rent(bufferSize), 0, bufferSize);
        }

        public void ReturnBuffer(ArraySegment<byte> buffer)
        {
            Fx.Assert(buffer.Offset % this.segmentSize == 0, "invalid offset");
            Fx.Assert(buffer.Count <= this.segmentSize, "invalid size");

            arrayPool.Return(buffer.Array);
        }
    }
}
