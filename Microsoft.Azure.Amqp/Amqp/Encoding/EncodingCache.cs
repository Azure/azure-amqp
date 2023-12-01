// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Encoding;

    static class EncodingCache
    {
        static readonly object[] boolCache = new object[] { true, false };
        static readonly BoxedCache<int> intCache = new BoxedCache<int>(17, EqualityComparer<int>.Default);
        static readonly BoxedCache<AmqpSymbol> symbolCache = new BoxedCache<AmqpSymbol>(40, SymbolComparer.Default);

        static readonly UlongCache performativeCodes = new UlongCache(0x10ul, 0x19ul);
        static readonly UlongCache outcomeCodes = new UlongCache(0x23ul, 0x29ul);
        static readonly UlongCache saslCodes = new UlongCache(0x40ul, 0x44ul);
        static readonly UlongCache messageCodes = new UlongCache(0x70ul, 0x78ul);
        static readonly UlongCache txnCodes = new UlongCache(0x30ul, 0x34ul);
        static readonly UlongCache errorCode = new UlongCache(0x1dul, 0x1dul);

        public static object Box(bool value)
        {
            return value ? boolCache[0] : boolCache[1];
        }

        public static object Box(int value)
        {
            return intCache.Box(value);
        }

        public static object Box(AmqpSymbol symbol)
        {
            if (symbol.Value == null)
            {
                return BoxedCache<AmqpSymbol>.Default;
            }

            return symbolCache.Box(symbol);
        }

        public static object Box(ulong code)
        {
            object obj;
            if (performativeCodes.TryGet(code, out obj))
            {
                return obj;
            }
            if (outcomeCodes.TryGet(code, out obj))
            {
                return obj;
            }
            if (saslCodes.TryGet(code, out obj))
            {
                return obj;
            }
            if (messageCodes.TryGet(code, out obj))
            {
                return obj;
            }
            if (txnCodes.TryGet(code, out obj))
            {
                return obj;
            }
            if (errorCode.TryGet(code, out obj))
            {
                return obj;
            }

            return code;
        }

        sealed class SymbolComparer : IEqualityComparer<AmqpSymbol>
        {
            public static readonly SymbolComparer Default = new SymbolComparer();

            bool IEqualityComparer<AmqpSymbol>.Equals(AmqpSymbol x, AmqpSymbol y)
            {
                return x.Equals(y);
            }

            int IEqualityComparer<AmqpSymbol>.GetHashCode(AmqpSymbol obj)
            {
                return obj.GetHashCode();
            }
        }

        sealed class BoxedCache<T> where T : struct
        {
            public static readonly object Default = default(T);

            struct Entry
            {
                public T Value;
                public object Boxed;
            }

            readonly IEqualityComparer<T> comparer;
            readonly Entry[] cache;

            public BoxedCache(int capacity, IEqualityComparer<T> comparer)
            {
                this.comparer = comparer;
                this.cache = new Entry[capacity];
            }

            public object Box(T t)
            {
                int slot = (int)((uint)this.comparer.GetHashCode(t) % (uint)this.cache.Length);
                Entry entry = this.cache[slot];
                if (entry.Boxed == null || !this.comparer.Equals(t, entry.Value))
                {
                    entry = new Entry { Value = t, Boxed = t };
                    this.cache[slot] = entry;
                }

                return entry.Boxed;
            }
        }

        sealed class UlongCache
        {
            readonly ulong start;
            readonly ulong end;
            readonly object[] cache;

            public UlongCache(ulong start, ulong end)
            {
                this.start = start;
                this.end = end;
                this.cache = new object[(int)(end - start + 1)];
                for (int i = 0; i < this.cache.Length; i++)
                {
                    this.cache[i] = start + (ulong)i;
                }
            }

            public bool TryGet(ulong value, out object obj)
            {
                if (value >= this.start && value <= this.end)
                {
                    int slot = (int)(value - this.start);
                    obj = this.cache[slot];
                    return true;
                }

                obj = null;
                return false;
            }
        }
    }
}
