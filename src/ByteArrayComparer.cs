// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Encoding;
    using System;
    using System.Collections.Generic;

    sealed class ByteArrayComparer : IEqualityComparer<ArraySegment<byte>>
    {
        static readonly ByteArrayComparer instance = new ByteArrayComparer();

        ByteArrayComparer()
        {
        }

        public static ByteArrayComparer Instance
        {
            get
            {
                return instance;
            }
        }

        public bool Equals(ArraySegment<byte> x, ArraySegment<byte> y)
        {
            return ByteArrayComparer.AreEqual(x, y);
        }

        public int GetHashCode(ArraySegment<byte> obj)
        {
            int num = obj.Count;
            unchecked
            {
                for (int i = 0; i < obj.Count; ++i)
                {
                    num = ((num << 4) - num) ^ obj.Array[i + obj.Offset];
                }
            }

            return num;
        }

        public static bool AreEqual(ArraySegment<byte> x, ArraySegment<byte> y)
        {
            if ((x.Array == null) || (y.Array == null))
            {
                return x.Array == null && null == y.Array;
            }

            if (x.Count != y.Count)
            {
                return false;
            }

            for (int i = 0; i < x.Count; i++)
            {
                if (x.Array[i + x.Offset] != y.Array[i + y.Offset])
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Use this as the comparer for an <see cref="AmqpMap"/> to compare the byte array content instead of a generic reference compare.
        /// </summary>
        public class MapKeyByteArrayComparer : IEqualityComparer<MapKey>
        {
            MapKeyByteArrayComparer()
            {
            }

            public static MapKeyByteArrayComparer Instance { get; } = new MapKeyByteArrayComparer();

            public bool Equals(MapKey x, MapKey y)
            {
                if ((x.Key == null) || (y.Key == null))
                {
                    return x.Key == null && y.Key == null;
                }

                if (!(x.Key is ArraySegment<byte>))
                {
                    throw new ArgumentException(CommonResources.GetString(CommonResources.InvalidType, nameof(MapKey), nameof(ArraySegment<byte>), x.Key.GetType()));
                }

                if (!(y.Key is ArraySegment<byte>))
                {
                    throw new ArgumentException(CommonResources.GetString(CommonResources.InvalidType, nameof(MapKey), nameof(ArraySegment<byte>), y.Key.GetType()));
                }

                return ByteArrayComparer.Instance.Equals((ArraySegment<byte>)x.Key, (ArraySegment<byte>)y.Key);
            }

            public int GetHashCode(MapKey obj)
            {
                if (!(obj.Key is ArraySegment<byte>))
                {
                    throw new ArgumentException(CommonResources.GetString(CommonResources.InvalidType, nameof(MapKey), nameof(ArraySegment<byte>), obj.Key?.GetType()));
                }

                return ByteArrayComparer.Instance.GetHashCode((ArraySegment<byte>)obj.Key);
            }
        }
    }
}
