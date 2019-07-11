// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Collections.Concurrent;

    abstract class InternalBufferManager
    {
        protected InternalBufferManager()
        {
        }

        public abstract byte[] TakeBuffer(int bufferSize);
        public abstract void ReturnBuffer(byte[] buffer);
        public abstract void Clear();

        public static InternalBufferManager CreatePooledBufferManager(long maxBufferPoolSize, int maxBufferSize)
        {
            if (maxBufferPoolSize == 0)
            {
                return GCBufferManager.Value;
            }

            Fx.Assert(maxBufferPoolSize > 0 && maxBufferSize >= 0, "bad params, caller should verify");
            return new PooledBufferManager(maxBufferPoolSize, maxBufferSize);
        }

        public static InternalBufferManager CreatePreallocatedBufferManager(int bufferSize, int maxCount)
        {
            return new PreallocatedBufferManager(bufferSize, maxCount);
        }

        public static byte[] AllocateByteArray(int size)
        {
            // This will be inlined in retail bits but provides a 
            // common entry point for debugging all buffer allocations 
            // and can be instrumented if necessary. 
            return new byte[size];
        }

        sealed class PreallocatedBufferManager : InternalBufferManager
        {
            readonly int bufferSize;
            readonly int maxCount;
            readonly ConcurrentQueue<byte[]> freeBuffers;

            internal PreallocatedBufferManager(int bufferSize, int maxCount)
            {
                this.bufferSize = bufferSize;
                this.maxCount = maxCount;
                this.freeBuffers = new ConcurrentQueue<byte[]>();
                for (int i = 0; i < this.maxCount; i++)
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
                while (this.freeBuffers.TryDequeue(out byte[] buffer))
                {
                }
            }
        }

        sealed class PooledBufferManager : InternalBufferManager
        {
            const int minBufferSize = 128;
            const int maxMissesBeforeTuning = 8;
            const int initialBufferCount = 1;
            readonly object tuningLock;

            readonly int[] bufferSizes;
            readonly BufferPool[] bufferPools;
            long remainingMemory;
            bool areQuotasBeingTuned;
            int totalMisses;

            public PooledBufferManager(long maxMemoryToPool, int maxBufferSize)
            {
                this.tuningLock = new object();
                this.remainingMemory = maxMemoryToPool;
                List<BufferPool> bufferPoolList = new List<BufferPool>();

                for (int bufferSize = minBufferSize; ;)
                {
                    long bufferCountLong = this.remainingMemory / bufferSize;

                    int bufferCount = bufferCountLong > int.MaxValue ? int.MaxValue : (int)bufferCountLong;

                    if (bufferCount > initialBufferCount)
                    {
                        bufferCount = initialBufferCount;
                    }

                    bufferPoolList.Add(BufferPool.CreatePool(bufferSize, bufferCount));

                    this.remainingMemory -= (long)bufferCount * bufferSize;

                    if (bufferSize >= maxBufferSize)
                    {
                        break;
                    }

                    long newBufferSizeLong = (long)bufferSize * 2;

                    if (newBufferSizeLong > (long)maxBufferSize)
                    {
                        bufferSize = maxBufferSize;
                    }
                    else
                    {
                        bufferSize = (int)newBufferSizeLong;
                    }
                }

                this.bufferPools = bufferPoolList.ToArray();
                this.bufferSizes = new int[bufferPools.Length];
                for (int i = 0; i < bufferPools.Length; i++)
                {
                    this.bufferSizes[i] = bufferPools[i].BufferSize;
                }
            }

            public override void Clear()
            {
                for (int i = 0; i < this.bufferPools.Length; i++)
                {
                    BufferPool bufferPool = this.bufferPools[i];
                    bufferPool.Clear();
                }
            }

            void ChangeQuota(ref BufferPool bufferPool, int delta)
            {
                BufferPool oldBufferPool = bufferPool;
                int newLimit = oldBufferPool.Limit + delta;
                BufferPool newBufferPool = BufferPool.CreatePool(oldBufferPool.BufferSize, newLimit);
                for (int i = 0; i < newLimit; i++)
                {
                    byte[] buffer = oldBufferPool.Take();
                    if (buffer == null)
                    {
                        break;
                    }
                    newBufferPool.Return(buffer);
                    newBufferPool.IncrementCount();
                }
                this.remainingMemory -= oldBufferPool.BufferSize * delta;
                bufferPool = newBufferPool;
            }

            void DecreaseQuota(ref BufferPool bufferPool)
            {
                ChangeQuota(ref bufferPool, -1);
            }

            int FindMostExcessivePool()
            {
                long maxBytesInExcess = 0;
                int index = -1;

                for (int i = 0; i < this.bufferPools.Length; i++)
                {
                    BufferPool bufferPool = this.bufferPools[i];

                    if (bufferPool.Peak < bufferPool.Limit)
                    {
                        long bytesInExcess = (bufferPool.Limit - bufferPool.Peak) * (long)bufferPool.BufferSize;

                        if (bytesInExcess > maxBytesInExcess)
                        {
                            index = i;
                            maxBytesInExcess = bytesInExcess;
                        }
                    }
                }

                return index;
            }

            int FindMostStarvedPool()
            {
                long maxBytesMissed = 0;
                int index = -1;

                for (int i = 0; i < this.bufferPools.Length; i++)
                {
                    BufferPool bufferPool = this.bufferPools[i];

                    if (bufferPool.Peak == bufferPool.Limit)
                    {
                        long bytesMissed = bufferPool.Misses * (long)bufferPool.BufferSize;

                        if (bytesMissed > maxBytesMissed)
                        {
                            index = i;
                            maxBytesMissed = bytesMissed;
                        }
                    }
                }

                return index;
            }

            BufferPool FindPool(int desiredBufferSize)
            {
                for (int i = 0; i < this.bufferSizes.Length; i++)
                {
                    if (desiredBufferSize <= this.bufferSizes[i])
                    {
                        return this.bufferPools[i];
                    }
                }

                return null;
            }

            void IncreaseQuota(ref BufferPool bufferPool)
            {
                ChangeQuota(ref bufferPool, 1);
            }

            public override void ReturnBuffer(byte[] buffer)
            {
                Fx.Assert(buffer != null, "caller must verify");

                BufferPool bufferPool = FindPool(buffer.Length);
                if (bufferPool != null)
                {
                    if (buffer.Length != bufferPool.BufferSize)
                    {
                        throw new ArgumentException("buffer", CommonResources.BufferIsNotRightSizeForBufferManager);
                    }

                    if (bufferPool.Return(buffer))
                    {
                        bufferPool.IncrementCount();
                    }
                }
            }

            public override byte[] TakeBuffer(int bufferSize)
            {
                Fx.Assert(bufferSize >= 0, "caller must ensure a non-negative argument");

                BufferPool bufferPool = FindPool(bufferSize);
                if (bufferPool != null)
                {
                    byte[] buffer = bufferPool.Take();
                    if (buffer != null)
                    {
                        bufferPool.DecrementCount();
                        return buffer;
                    }
                    if (bufferPool.Peak == bufferPool.Limit)
                    {
                        bufferPool.Misses++;
                        if (++totalMisses >= maxMissesBeforeTuning)
                        {
                            TuneQuotas();
                        }
                    }
                    return InternalBufferManager.AllocateByteArray(bufferPool.BufferSize);
                }
                else
                {
                    return InternalBufferManager.AllocateByteArray(bufferSize);
                }
            }

            void TuneQuotas()
            {
                if (this.areQuotasBeingTuned)
                {
                    return;
                }

                bool lockHeld = false;
                try
                {
                    Monitor.TryEnter(this.tuningLock, ref lockHeld);

                    // Don't bother if another thread already has the lock
                    if (!lockHeld || this.areQuotasBeingTuned)
                    {
                        return;
                    }

                    this.areQuotasBeingTuned = true;
                }
                finally
                {
                    if (lockHeld)
                    {
                        Monitor.Exit(this.tuningLock);
                    }
                }

                // find the "poorest" pool
                int starvedIndex = FindMostStarvedPool();
                if (starvedIndex >= 0)
                {
                    BufferPool starvedBufferPool = this.bufferPools[starvedIndex];

                    if (this.remainingMemory < starvedBufferPool.BufferSize)
                    {
                        // find the "richest" pool
                        int excessiveIndex = FindMostExcessivePool();
                        if (excessiveIndex >= 0)
                        {
                            // steal from the richest
                            DecreaseQuota(ref this.bufferPools[excessiveIndex]);
                        }
                    }

                    if (this.remainingMemory >= starvedBufferPool.BufferSize)
                    {
                        // give to the poorest
                        IncreaseQuota(ref this.bufferPools[starvedIndex]);
                    }
                }

                // reset statistics
                for (int i = 0; i < this.bufferPools.Length; i++)
                {
                    BufferPool bufferPool = this.bufferPools[i];
                    bufferPool.Misses = 0;
                }

                this.totalMisses = 0;
                this.areQuotasBeingTuned = false;
            }

            abstract class BufferPool
            {
                readonly int bufferSize;
                readonly int limit;

                int count;
                int misses;
                int peak;

                public BufferPool(int bufferSize, int limit)
                {
                    this.bufferSize = bufferSize;
                    this.limit = limit;
                }

                public int BufferSize
                {
                    get { return this.bufferSize; }
                }

                public int Limit
                {
                    get { return this.limit; }
                }

                public int Misses
                {
                    get { return this.misses; }
                    set { this.misses = value; }
                }

                public int Peak
                {
                    get { return this.peak; }
                }

                public void Clear()
                {
                    this.OnClear();
                    this.count = 0;
                }

                public void DecrementCount()
                {
                    int newValue = this.count - 1;
                    if (newValue >= 0)
                    {
                        this.count = newValue;
                    }
                }

                public void IncrementCount()
                {
                    int newValue = this.count + 1;
                    if (newValue <= this.limit)
                    {
                        this.count = newValue;
                        if (newValue > this.peak)
                        {
                            this.peak = newValue;
                        }
                    }
                }

                internal abstract byte[] Take();
                internal abstract bool Return(byte[] buffer);
                internal abstract void OnClear();

                internal static BufferPool CreatePool(int bufferSize, int limit)
                {
                    // To avoid many buffer drops during training of large objects which
                    // get allocated on the LOH, we use the LargeBufferPool and for 
                    // bufferSize < 85000, the SynchronizedPool. There is a 12 or 24(x64)
                    // byte overhead for an array so we use 85000-24=84976 as the limit
                    if (bufferSize < 84976)
                    {
                        return new SynchronizedBufferPool(bufferSize, limit);
                    }
                    else
                    {
                        return new LargeBufferPool(bufferSize, limit);
                    }
                }

                sealed class SynchronizedBufferPool : BufferPool
                {
                    readonly SynchronizedPool<byte[]> innerPool;

                    internal SynchronizedBufferPool(int bufferSize, int limit)
                        : base(bufferSize, limit)
                    {
                        this.innerPool = new SynchronizedPool<byte[]>(limit);
                    }

                    internal override void OnClear()
                    {
                        this.innerPool.Clear();
                    }

                    internal override byte[] Take()
                    {
                        return this.innerPool.Take();
                    }

                    internal override bool Return(byte[] buffer)
                    {
                        return this.innerPool.Return(buffer);
                    }
                }

                sealed class LargeBufferPool : BufferPool
                {
                    readonly Stack<byte[]> items;

                    internal LargeBufferPool(int bufferSize, int limit)
                        : base(bufferSize, limit)
                    {
                        this.items = new Stack<byte[]>(limit);
                    }

                    object ThisLock
                    {
                        get
                        {
                            return this.items;
                        }
                    }

                    internal override void OnClear()
                    {
                        lock (ThisLock)
                        {
                            this.items.Clear();
                        }
                    }

                    internal override byte[] Take()
                    {
                        lock (ThisLock)
                        {
                            if (this.items.Count > 0)
                            {
                                return this.items.Pop();
                            }
                        }

                        return null;
                    }

                    internal override bool Return(byte[] buffer)
                    {
                        lock (ThisLock)
                        {
                            if (this.items.Count < this.Limit)
                            {
                                this.items.Push(buffer);
                                return true;
                            }
                        }

                        return false;
                    }
                }
            }
        }

        sealed class GCBufferManager : InternalBufferManager
        {
            static readonly GCBufferManager value = new GCBufferManager();

            GCBufferManager()
            {
            }

            public static GCBufferManager Value
            {
                get { return value; }
            }

            public override void Clear()
            {
            }

            public override byte[] TakeBuffer(int bufferSize)
            {
                return InternalBufferManager.AllocateByteArray(bufferSize);
            }

            public override void ReturnBuffer(byte[] buffer)
            {
                // do nothing, GC will reclaim this buffer
            }
        }
    }
}
