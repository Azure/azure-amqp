// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Buffers;
    using System.Collections.Concurrent;

    abstract class InternalBufferManager
    {
        protected InternalBufferManager()
        {
        }

        public abstract byte[] TakeBuffer(int bufferSize);
        public abstract void ReturnBuffer(byte[] buffer);
        public abstract void Clear();

        public static InternalBufferManager CreatePooledBufferManager()
        {
            return new PooledBufferManager();
        }

        public static InternalBufferManager CreatePreallocatedBufferManager(int bufferSize, int maxCount)
        {
            return new PreallocatedBufferManager(bufferSize, maxCount);
        }

        sealed class PreallocatedBufferManager : InternalBufferManager
        {
            readonly int bufferSize;
            readonly ConcurrentQueue<byte[]> freeBuffers;

            internal PreallocatedBufferManager(int bufferSize, int maxCount)
            {
                this.bufferSize = bufferSize;
                this.freeBuffers = new ConcurrentQueue<byte[]>();
                for (int i = 0; i < maxCount; i++)
                {
                    this.freeBuffers.Enqueue(new byte[this.bufferSize]);
                }
            }

            public override byte[] TakeBuffer(int bufferSize)
            {
                byte[] returnedBuffer = null;
                if (bufferSize <= this.bufferSize)
                {
                    this.freeBuffers.TryDequeue(out returnedBuffer);
                }

                return returnedBuffer;
            }

            public override void ReturnBuffer(byte[] buffer)
            {
                if (buffer.Length == this.bufferSize)
                {
                    this.freeBuffers.Enqueue(buffer);
                }
            }

            public override void Clear()
            {
                while (this.freeBuffers.TryDequeue(out byte[] _))
                {
                }
            }
        }

        sealed class PooledBufferManager : InternalBufferManager
        {
            public override void Clear()
            {
            }

            public override void ReturnBuffer(byte[] buffer)
            {
                Fx.Assert(buffer != null, "caller must verify");

                ArrayPool<byte>.Shared.Return(buffer);
            }

            public override byte[] TakeBuffer(int bufferSize)
            {
                Fx.Assert(bufferSize >= 0, "caller must ensure a non-negative argument");

                return ArrayPool<byte>.Shared.Rent(bufferSize);
            }
        }
    }
}
