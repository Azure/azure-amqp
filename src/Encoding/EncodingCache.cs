// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;

    static class EncodingCache
    {
        static readonly object[] boolCache = new object[] { true, false };
        static readonly object[] intCache = new object[] { 0, 1, 2, 3, 4, 5, 6, 7 };
        static readonly KeyValueCache<AmqpSymbol, object> boxedSymbolCache = new KeyValueCache<AmqpSymbol, object>(
            capacity: 43,
            comparer: SymbolComparer.Default,
            keyFunc: s => s,
            valueFunc: s => (object)s);
        static readonly KeyValueCache<ArraySegment<byte>, string> encodedSymbolCache = new KeyValueCache<ArraySegment<byte>, string>(
            capacity: 43,
            comparer: ByteArrayComparer.Instance,
            keyFunc: a => Copy(a),
            valueFunc: a => System.Text.Encoding.ASCII.GetString(a.Array, a.Offset, a.Count));

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
            if (value >= 0 && value < intCache.Length)
            {
                return intCache[value];
            }

            return value;
        }

        public static object Box(AmqpSymbol symbol)
        {
            if (symbol.Value == null)
            {
                return KeyValueCache<AmqpSymbol, object>.Default;
            }

            return boxedSymbolCache.Get(symbol);
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

        public static AmqpSymbol GetSymbol(ArraySegment<byte> bytes)
        {
            return encodedSymbolCache.Get(bytes);
        }

        static ArraySegment<byte> Copy(ArraySegment<byte> a)
        {
            var b = new byte[a.Count];
            Buffer.BlockCopy(a.Array, a.Offset, b, 0, a.Count);
            return new ArraySegment<byte>(b);
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

        sealed class KeyValueCache<TKey, TValue> where TValue : class
        {
            public static readonly object Default = default(TValue);

            struct Entry
            {
                public TKey Key;
                public TValue Value;

                public bool TrySet(TKey key, TValue value, Func<TKey, TKey> keyFunc)
                {
                    if (Interlocked.CompareExchange(ref this.Value, value, null) == null)
                    {
                        this.Key = keyFunc(key);
                        return true;
                    }

                    return false;
                }
            }

            readonly IEqualityComparer<TKey> comparer;
            readonly Func<TKey, TKey> keyFunc;
            readonly Func<TKey, TValue> valueFunc;
            readonly Entry[] cache1;
            readonly Entry[] cache2;
            readonly Entry[] cache3;

            public KeyValueCache(int capacity, IEqualityComparer<TKey> comparer, Func<TKey, TKey> keyFunc, Func<TKey, TValue> valueFunc)
            {
                this.comparer = comparer;
                this.keyFunc = keyFunc;
                this.valueFunc = valueFunc;
                this.cache1 = new Entry[29];
                this.cache2 = new Entry[11];
                this.cache3 = new Entry[5];
            }

            public TValue Get(TKey key)
            {
                int hash = this.comparer.GetHashCode(key);
                TValue value;
                if (this.TryGet(key, hash, this.cache1, out value))
                {
                    return value;
                }

                if (this.TryGet(key, hash, this.cache2, out value))
                {
                    return value;
                }

                if (this.TryGet(key, hash, this.cache3, out value))
                {
                    return value;
                }

                // The entry is occupied by another value.
                return this.valueFunc(key);
            }

            bool TryGet(TKey key, int hash, Entry[] entries, out TValue value)
            {
                int slot = (int)((uint)hash % (uint)entries.Length);
                Entry entry = entries[slot];
                if (entry.Value == null)
                {
                    value = this.valueFunc(key);
                    // Do not use 'entry' which is a copy of the item
                    if (entries[slot].TrySet(key, value, this.keyFunc))
                    {
                        return true;
                    }
                }
                else if (this.comparer.Equals(key, entry.Key))
                {
                    value = entry.Value;
                    return true;
                }

                value = null;
                return false;
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
