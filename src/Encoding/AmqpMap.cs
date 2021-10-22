// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// This class implements an AMQP map.
    /// </summary>
    public sealed class AmqpMap : Dictionary<MapKey, object>
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        public AmqpMap() : base()
        {
        }

        /// <summary>
        /// Initializes the object from a dictionary.
        /// </summary>
        /// <param name="value">The dictionary.</param>
        public AmqpMap(IDictionary value)
            : this()
        {
            if (value != null)
            {
                foreach (DictionaryEntry entry in value)
                {
                    this.Add(entry.Key is MapKey ? (MapKey)entry.Key : new MapKey(entry.Key), entry.Value);
                }
            }
        }

        /// <summary>
        /// Initializes the object from a dictionary and a given comparer.
        /// </summary>
        /// <param name="value">The dictionary.</param>
        /// <param name="comparer">The equality comparer.</param>
        public AmqpMap(IDictionary value, IEqualityComparer<MapKey> comparer)
            : base(comparer)
        {
            if (value != null)
            {
                foreach (DictionaryEntry entry in value)
                {
                    this.Add(entry.Key is MapKey ? (MapKey)entry.Key : new MapKey(entry.Key), entry.Value);
                }
            }
        }

        /// <summary>
        /// Gets a value from the map for a given key.
        /// </summary>
        /// <typeparam name="TValue">The expected type of the value.</typeparam>
        /// <param name="key">The key to lookup.</param>
        /// <param name="value">The returned value.</param>
        /// <returns>True if the key is found and the type matches; false otherwise.</returns>
        /// <remarks>This method returns false if the key exists but the value type
        /// does not match the expected type. Use the indexer to access the value
        /// if this is not the expected behavior.</remarks>
        public bool TryGetValue<TValue>(MapKey key, out TValue value)
        {
            object obj;
            if (base.TryGetValue(key, out obj))
            {
                if (obj == null)
                {
                    value = default(TValue);
                    return true;
                }

                if (obj is TValue)
                {
                    value = (TValue)obj;
                    return true;
                }
            }

            value = default(TValue);
            return false;
        }

        /// <summary>
        /// Removes a value from the map for a given key.
        /// </summary>
        /// <typeparam name="TValue">The expected type of the value.</typeparam>
        /// <param name="key">The key to lookup.</param>
        /// <param name="value">The value to remove.</param>
        /// <returns>True if the key is found and the type matches; false otherwise.</returns>
        public bool TryRemoveValue<TValue>(MapKey key, out TValue value)
        {
            if (this.TryGetValue(key, out value))
            {
                this.Remove(key);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Gets a string to represent the object.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append('[');
            bool firstItem = true;
            foreach (KeyValuePair<MapKey, object> pair in this)
            {
                if (firstItem)
                {
                    firstItem = false;
                }
                else
                {
                    sb.Append(',');
                }

                sb.AppendFormat(CultureInfo.InvariantCulture, "{0}:{1}", pair.Key, pair.Value);
            }

            sb.Append(']');
            return sb.ToString();
        }
    }
}
