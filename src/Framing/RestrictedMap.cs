// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Collections;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// The base class of a map with restricted key types.
    /// </summary>
    public abstract class RestrictedMap : IEnumerable<KeyValuePair<MapKey, object>>
    {
        AmqpMap innerMap;

        internal AmqpMap InnerMap
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

        internal void SetMap(AmqpMap map)
        {
            this.innerMap = map;
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return this.InnerMap.ToString();
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
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

    /// <summary>
    /// The base class of a map with restricted key types.
    /// </summary>
    /// <typeparam name="TKey">The key type.</typeparam>
    public abstract class RestrictedMap<TKey> : RestrictedMap
    {
        /// <summary>
        /// Returns an <see cref="AmqpMap"/>.
        /// </summary>
        /// <param name="restrictedMap">The restricted map object.</param>
        public static implicit operator AmqpMap(RestrictedMap<TKey> restrictedMap)
        {
            return restrictedMap == null ? null : restrictedMap.InnerMap;
        }

        /// <summary>
        /// Gets or sets a value for a given key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>The value. Null if the key doesn't exist.</returns>
        public object this[TKey key]
        {
            get { return this.InnerMap[new MapKey(key)]; }
            set { this.InnerMap[new MapKey(key)] = value; }
        }

        /// <summary>
        /// Gets or sets a value for a given key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>The value. Null if the key doesn't exist.</returns>
        public object this[MapKey key]
        {
            get { return this.InnerMap[key]; }
            set { this.InnerMap[key] = value; }
        }

        /// <summary>
        /// Gets a value from the map. See <see cref="AmqpMap.TryGetValue{TValue}(MapKey, out TValue)"/>.
        /// </summary>
        /// <typeparam name="TValue">The expected value type.</typeparam>
        /// <param name="key">The key.</param>
        /// <param name="value">The returned value.</param>
        /// <returns>True if the key is found and the type matches; false otherwise.</returns>
        public bool TryGetValue<TValue>(TKey key, out TValue value)
        {
            return this.InnerMap.TryGetValue(new MapKey(key), out value);
        }

        /// <summary>
        /// Gets a value from the map. See <see cref="AmqpMap.TryGetValue{TValue}(MapKey, out TValue)"/>.
        /// </summary>
        /// <typeparam name="TValue">The expected value type.</typeparam>
        /// <param name="key">The key.</param>
        /// <param name="value">The returned value.</param>
        /// <returns>True if the key is found and the type matches; false otherwise.</returns>
        public bool TryGetValue<TValue>(MapKey key, out TValue value)
        {
            return this.InnerMap.TryGetValue(key, out value);
        }

        /// <summary>
        /// Gets the value of a key and removes it if the key exists.
        /// </summary>
        /// <typeparam name="TValue">The expected value type.</typeparam>
        /// <param name="key">The key.</param>
        /// <param name="value">The returned value.</param>
        /// <returns>True if the key is found and the type matches; false otherwise.</returns>
        public bool TryRemoveValue<TValue>(TKey key, out TValue value)
        {
            return this.InnerMap.TryRemoveValue(new MapKey(key), out value);
        }

        /// <summary>
        /// Adds the specified key and value to the map.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add.</param>
        public void Add(TKey key, object value)
        {
            this.InnerMap.Add(new MapKey(key), value);
        }

        /// <summary>
        /// Adds the specified key and value to the map.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add.</param>
        public void Add(MapKey key, object value)
        {
            this.InnerMap.Add(key, value);
        }

        /// <summary>
        /// Merges key-value items from another map. If a key exists, its value
        /// is overwritten.
        /// </summary>
        /// <param name="map">The source map.</param>
        public void Merge(RestrictedMap<TKey> map)
        {
            foreach (var kvp in map)
            {
                this[kvp.Key] = kvp.Value;
            }
        }
    }

    /// <summary>
    /// Defines the AMQP fields.
    /// </summary>
    public sealed class Fields : RestrictedMap<AmqpSymbol>
    {
    }

    /// <summary>
    /// Defines the AMQP filter-set.
    /// </summary>
    public sealed class FilterSet : RestrictedMap<AmqpSymbol>
    {
    }

    /// <summary>
    /// Defines the AMQP properties map whose key is string.
    /// </summary>
    public sealed class PropertiesMap : RestrictedMap<string>
    {
    }

    /// <summary>
    /// Defines the AMQP annotations map whose key is symbol.
    /// </summary>
    public sealed class Annotations : RestrictedMap<AmqpSymbol>
    {
    }
}
