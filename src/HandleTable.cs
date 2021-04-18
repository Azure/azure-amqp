// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Concurrent;
    using System.Threading;

    sealed class HandleTable<T> where T : class
    {
        const int InitialCapacity = 4;
        uint maxHandle;
        int count;
        private ConcurrentDictionary<uint, T> handles;

        public HandleTable(uint maxHandle)
        {
            this.maxHandle = maxHandle;
            this.handles = new ConcurrentDictionary<uint, T>(Environment.ProcessorCount, InitialCapacity);
        }

        public IEnumerable<T> Values => this.handles.Values;

        public void SetMaxHandle(uint maxHandle)
        {
            this.maxHandle = maxHandle;
        }

        public bool TryGetObject(uint handle, out T value)
        {
            return handles.TryGetValue(handle, out value);
        }

        public uint Add(T value)
        {
            if (count > this.maxHandle)
            {
                throw new AmqpException(AmqpErrorCode.ResourceLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpHandleExceeded, this.maxHandle));
            }

            uint index = (uint)this.count;
            if (!handles.TryAdd(index, value))
            {
                // count and actual handles go out of sync. there must be a bug
                throw new AmqpException(AmqpErrorCode.InternalError, null);
            }
            Interlocked.Increment(ref count);
            return index;
        }

        public void Add(uint handle, T value)
        {
            if (handle > this.maxHandle)
            {
                throw new AmqpException(AmqpErrorCode.ResourceLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpHandleExceeded, this.maxHandle));
            }

            if (!handles.TryAdd(handle, value))
            {
                throw new AmqpException(AmqpErrorCode.HandleInUse, AmqpResources.GetString(AmqpResources.AmqpHandleInUse, handle, this.handles[handle]));
            }

            Interlocked.Increment(ref count);
        }

        public void Remove(uint handle)
        {
            if (handles.TryRemove(handle, out _))
            {
                Interlocked.Decrement(ref count);
            }
        }

        public void Clear()
        {
            Interlocked.Exchange(ref count, 0);
            handles.Clear();
        }
    }
}
