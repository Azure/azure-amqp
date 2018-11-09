// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Collections;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class RestrictedMap : IEnumerable<KeyValuePair<MapKey, object>>
    {
        AmqpMap innerMap;

        protected AmqpMap InnerMap
        {
            get 
            {
                if (this.innerMap == null)
                {
                    this.innerMap = new AmqpMap();
                }

                return this.innerMap; 
            }
        }

        public void SetMap(AmqpMap map)
        {
            this.innerMap = map;
        }

        public override string ToString()
        {
            return this.InnerMap.ToString();
        }

        public IEnumerator<KeyValuePair<MapKey, object>> GetEnumerator()
        {
            IEnumerable<KeyValuePair<MapKey, object>> map = this.InnerMap;
            return map.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }

    public abstract class RestrictedMap<TKey> : RestrictedMap
    {
        public static implicit operator AmqpMap(RestrictedMap<TKey> restrictedMap)
        {
            return restrictedMap == null ? null : restrictedMap.InnerMap;
        }

        public object this[TKey key]
        {
            get { return this.InnerMap[new MapKey(key)]; }
            set { this.InnerMap[new MapKey(key)] = value; }
        }

        public object this[MapKey key]
        {
            get { return this.InnerMap[key]; }
            set { this.InnerMap[key] = value; }
        }

        public bool TryGetValue<TValue>(TKey key, out TValue value)
        {
            return this.InnerMap.TryGetValue(new MapKey(key), out value);
        }

        public bool TryGetValue<TValue>(MapKey key, out TValue value)
        {
            return this.InnerMap.TryGetValue(key, out value);
        }

        public bool TryRemoveValue<TValue>(TKey key, out TValue value)
        {
            return this.InnerMap.TryRemoveValue(new MapKey(key), out value);
        }

        public void Add(TKey key, object value)
        {
            this.InnerMap.Add(new MapKey(key), value);
        }

        public void Add(MapKey key, object value)
        {
            this.InnerMap.Add(key, value);
        }

        public void Merge(RestrictedMap<TKey> map)
        {
            foreach (var kvp in map)
            {
                this[kvp.Key] = kvp.Value;
            }
        }
    }

    public sealed class Fields : RestrictedMap<AmqpSymbol>
    {
    }

    public sealed class FilterSet : RestrictedMap<AmqpSymbol>
    {
    }

    public sealed class PropertiesMap : RestrictedMap<string>
    {
    }

    public sealed class Annotations : RestrictedMap<AmqpSymbol>
    {
    }
}
