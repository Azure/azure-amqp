// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    sealed class HandleTable<T> where T : class
    {
        const int InitialCapacity = 4;
        const int ResetThreshold = 4 * 1024;
        uint maxHandle;
        T[] handleArray;
        int count;

        public HandleTable(uint maxHandle)
        {
            this.maxHandle = maxHandle;
            this.handleArray = new T[InitialCapacity];
        }

        public IEnumerable<T> Values
        {
            get
            {
                var values = new List<T>(this.count);
                foreach (T t in this.handleArray)
                {
                    if (t != null)
                    {
                        values.Add(t);
                    }
                }

                return values;
            }
        }

        public void SetMaxHandle(uint maxHandle)
        {
            this.maxHandle = maxHandle;
        }

        public IEnumerator<T> GetSafeEnumerator()
        {
            return new SafeEnumerator(this);
        }

        public bool TryGetObject(uint handle, out T value)
        {
            value = null;
            if (handle < this.handleArray.Length)
            {
                value = this.handleArray[(int)handle];
            }

            return value != null;
        }

        public uint Add(T value)
        {
            if (this.count > this.maxHandle)
            {
                throw new AmqpException(AmqpErrorCode.ResourceLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpHandleExceeded, this.maxHandle));
            }
            else if (this.count >= this.handleArray.Length)
            {
                this.GrowHandleArray(this.handleArray.Length * 2);

                int index = this.count;
                this.handleArray[index] = value;
                this.count++;
                return (uint)index;
            }
            else
            {
                for (int i = 0; i < this.handleArray.Length; i++)
                {
                    if (this.handleArray[i] == null)
                    {
                        this.handleArray[i] = value;
                        this.count++;
                        return (uint)i;
                    }
                }

                // count and actual handles go out of sync. there must be a bug
                throw new AmqpException(AmqpErrorCode.InternalError, null);
            }
        }

        public void Add(uint handle, T value)
        {
            if (handle > this.maxHandle)
            {
                throw new AmqpException(AmqpErrorCode.ResourceLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpHandleExceeded, this.maxHandle));
            }

            int index = (int)handle;
            if (index >= this.handleArray.Length)
            {
                int capacity = UpperPowerOfTwo(this.handleArray.Length, index);
                this.GrowHandleArray(capacity);
            }
            else if (this.handleArray[index] != null)
            {
                throw new AmqpException(AmqpErrorCode.HandleInUse, AmqpResources.GetString(AmqpResources.AmqpHandleInUse, handle, this.handleArray[index]));
            }

            this.handleArray[(int)handle] = value;
            this.count++;
        }

        public void Remove(uint handle)
        {
            int index = (int)handle;
            if (index < this.handleArray.Length && this.handleArray[index] != null)
            {
                this.handleArray[index] = null;
                this.count--;

                // trim if necessary
                if (this.count == 0 && this.handleArray.Length >= ResetThreshold)
                {
                    this.handleArray = new T[InitialCapacity];
                }
            }
        }

        public void Clear()
        {
            this.count = 0;
            for (int i = 0; i < this.handleArray.Length; i++)
            {
                this.handleArray[i] = null;
            }
        }

        void GrowHandleArray(int capacity)
        {
            Fx.Assert(capacity > this.handleArray.Length, "cannot grow with smaller capacity");
            T[] expanded = new T[capacity];
            Array.Copy(this.handleArray, expanded, this.handleArray.Length);
            this.handleArray = expanded;
        }

        static int UpperPowerOfTwo(int from, int num)
        {
            // assuming from is already power of 2
            while (from <= num)
            {
                from *= 2;
            }

            return from;
        }

        /// <summary>
        /// Use this only if the enumeration is best effort only
        /// </summary>
        sealed class SafeEnumerator : IEnumerator<T>
        {
            readonly HandleTable<T> table;
            int index;
            T current;

            public SafeEnumerator(HandleTable<T> table)
            {
                this.table = table;
                this.index = -1;
            }

            T IEnumerator<T>.Current
            {
                get { return this.current; }
            }

            object IEnumerator.Current
            {
                get { return this.current; }
            }

            bool IEnumerator.MoveNext()
            {
                try
                {
                    for (this.index++; this.index < this.table.handleArray.Length; this.index++)
                    {
                        this.current = this.table.handleArray[this.index];
                        if (this.current != null)
                        {
                            return true;
                        }
                    }
                }
                catch (IndexOutOfRangeException)
                {
                    // the array might be shrinked in rare cases
                }

                this.current = null;
                return false;
            }

            void IEnumerator.Reset()
            {
                this.index = -1;
                this.current = null;
            }

            void IDisposable.Dispose()
            {
            }
        }
    }
}
