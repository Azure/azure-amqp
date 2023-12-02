// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;

    public class AmqpMap : IEnumerable<KeyValuePair<MapKey, object>>
    {
        Dictionary<MapKey, object> value;

        public AmqpMap()
        {
            this.value = new Dictionary<MapKey, object>();
        }

        public AmqpMap(IDictionary<MapKey, object> value)
        {
            if (value is Dictionary<MapKey, object> dictionaryValue)
            {
                this.value = dictionaryValue;
            }
            else
            {
                this.value = new Dictionary<MapKey, object>(value);
            }
        }

        public AmqpMap(IDictionary value)
            : this()
        {
            foreach (DictionaryEntry entry in value)
            {
                this.value.Add(new MapKey(entry.Key), entry.Value);
            }
        }

        public int Count
        {
            get { return this.value.Count; }
        }

        public int ValueSize
        {
            get
            {
                return MapEncoding.GetValueSize(this);
            }
        }

        public object this[MapKey key]
        {
            get
            {
                object obj;
                if (this.value.TryGetValue(key, out obj))
                {
                    return obj;
                }

                return null;
            }

            set
            {
                this.value[key] = value;
            }
        }

        public bool TryGetValue<TValue>(MapKey key, out TValue value)
        {
            object obj;
            if (this.value.TryGetValue(key, out obj))
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

        public bool TryRemoveValue<TValue>(MapKey key, out TValue value)
        {
            if (this.TryGetValue(key, out value))
            {
                this.value.Remove(key);
                return true;
            }

            return false;
        }

        public void Add(MapKey key, object value)
        {
            this.value.Add(key, value);
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append('[');
            bool firstItem = true;
            foreach (KeyValuePair<MapKey, object> pair in this.value)
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

        public Dictionary<MapKey, object>.Enumerator GetEnumerator() => this.value.GetEnumerator();

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.value.GetEnumerator();
        }

        IEnumerator<KeyValuePair<MapKey, object>> IEnumerable<KeyValuePair<MapKey, object>>.GetEnumerator()
        {
            return this.value.GetEnumerator();
        }
    }
}
