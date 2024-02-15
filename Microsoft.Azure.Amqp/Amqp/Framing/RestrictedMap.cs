// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class RestrictedMap : AmqpMap
    {
        [Obsolete]
        protected AmqpMap InnerMap => this;

        [Obsolete]
        public void SetMap(AmqpMap map)
        {
        }

        public override string ToString()
        {
            return base.ToString();
        }
    }

    public abstract class RestrictedMap<TKey> : RestrictedMap
    {
        public object this[TKey key]
        {
            get { return this[this.GetKey(key)]; }
            set { this[this.GetKey(key)] = value; }
        }

        public bool TryGetValue<TValue>(TKey key, out TValue value)
        {
            return this.TryGetValue(this.GetKey(key), out value);
        }

        public bool TryRemoveValue<TValue>(TKey key, out TValue value)
        {
            return this.TryRemoveValue(this.GetKey(key), out value);
        }

        public void Add(TKey key, object value)
        {
            this.Add(this.GetKey(key), value);
        }

        public void Merge(RestrictedMap<TKey> map)
        {
            foreach (var kvp in map)
            {
                this[kvp.Key] = kvp.Value;
            }
        }

        protected virtual MapKey GetKey(TKey key)
        {
            return new MapKey(key);
        }
    }

    public sealed class Fields : RestrictedMap<AmqpSymbol>
    {
        protected override MapKey GetKey(AmqpSymbol key)
        {
            return new MapKey(EncodingCache.Box(key));
        }
    }

    public sealed class FilterSet : RestrictedMap<AmqpSymbol>
    {
        protected override MapKey GetKey(AmqpSymbol key)
        {
            return new MapKey(EncodingCache.Box(key));
        }
    }

    public sealed class PropertiesMap : RestrictedMap<string>
    {
    }

    public sealed class Annotations : RestrictedMap<AmqpSymbol>
    {
        protected override MapKey GetKey(AmqpSymbol key)
        {
            return new MapKey(EncodingCache.Box(key));
        }
    }
}
