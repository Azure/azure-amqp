// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.Concurrent;
    using System.Threading;

    sealed class HandleTable<T> where T : class
    {
        const int InitialCapacity = 4;
        ConcurrentDictionary<uint, T> handleDictionary;
        uint maxHandle;
        int nextAvailableHandle;

        public HandleTable(uint maxHandle)
        {
            this.maxHandle = maxHandle;
            this.handleDictionary = new ConcurrentDictionary<uint, T>(Environment.ProcessorCount, InitialCapacity);
        }

        public ICollection<T> Values => this.handleDictionary.Values;

        public void SetMaxHandle(uint maxHandle)
        {
            this.maxHandle = maxHandle;
        }

        public IEnumerator<T> GetSafeEnumerator()
        {
            return new SafeEnumerator(this.handleDictionary.GetEnumerator());
        }

        public bool TryGetObject(uint handle, out T value)
        {
            return this.handleDictionary.TryGetValue(handle, out value);
        }

        public uint Add(T value)
        {
            if (this.handleDictionary.Count >= this.maxHandle + 1)
            {
                throw new AmqpException(AmqpErrorCode.ResourceLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpHandleExceeded, this.maxHandle));
            }

            var startingHandle = (uint)Volatile.Read(ref nextAvailableHandle);
            var handle = startingHandle;
            var added = false;
        
            for (uint i = 0; i <= this.maxHandle && !added; i++)
            {
                added = this.handleDictionary.TryAdd(handle, value);
            
                if (added)
                {
                    Volatile.Write(ref nextAvailableHandle, (int)(handle >= maxHandle ? 0 : handle + 1));
                    return handle;
                }
            
                handle = handle >= maxHandle ? 0 : handle + 1;
            }

            // This should never happen if count < maxHandle+1
            throw new AmqpException(AmqpErrorCode.InternalError, "No handles available despite count check");
        }

        public void Add(uint handle, T value)
        {
            if (handle > this.maxHandle)
            {
                throw new AmqpException(AmqpErrorCode.ResourceLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpHandleExceeded, this.maxHandle));
            }

            var added = this.handleDictionary.GetOrAdd(handle, value);
            if (!ReferenceEquals(added, value))
            {
                throw new AmqpException(AmqpErrorCode.HandleInUse, AmqpResources.GetString(AmqpResources.AmqpHandleInUse, handle, added));
            }
        }

        public void Remove(uint handle)
        {
            if (this.handleDictionary.TryRemove(handle, out _))
            {
                var current = Volatile.Read(ref nextAvailableHandle);
                var intHandle = (int)handle;
                while (intHandle < current)
                {
                    var exchangeResult = Interlocked.CompareExchange(ref nextAvailableHandle, intHandle, current);
                    if (exchangeResult == current)
                    {
                        break;
                    }
                    current = exchangeResult;
                }
            }
        }

        public void Clear()
        {
            handleDictionary.Clear();
            nextAvailableHandle = 0;
        }

        // SafeEnumerator implementation
        sealed class SafeEnumerator : IEnumerator<T>
        {
            readonly IEnumerator<KeyValuePair<uint, T>> enumerator;

            public SafeEnumerator(IEnumerator<KeyValuePair<uint, T>> enumerator)
            {
                this.enumerator = enumerator;
            }

            T IEnumerator<T>.Current => this.enumerator.Current.Value;

            object IEnumerator.Current => this.enumerator.Current.Value;

            bool IEnumerator.MoveNext() => this.enumerator.MoveNext();

            void IEnumerator.Reset() => this.enumerator.Reset();

            void IDisposable.Dispose() => this.enumerator.Dispose();
        }
    }
}