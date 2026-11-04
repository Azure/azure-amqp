// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Encoding;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;

    /// <summary>
    /// Use this as the comparer for an <see cref="AmqpMap"/> to compare the byte array content instead of a generic reference compare.
    /// </summary>
    sealed class MapKeyByteArrayComparer : IEqualityComparer<MapKey>
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

            if (!(x.Key is ArraySegment<byte> xKey))
            {
                ThrowInvalidMapKey(x.Key.GetType());
                xKey = default;
            }

            if (!(y.Key is ArraySegment<byte> yKey))
            {
                ThrowInvalidMapKey(y.Key.GetType());
                yKey = default;
            }

            return ByteArrayComparer.Instance.Equals(xKey, yKey);
        }

        public int GetHashCode(MapKey obj)
        {
            if (!(obj.Key is ArraySegment<byte>))
            {
                ThrowInvalidMapKey(obj.Key?.GetType());
            }

            return ByteArrayComparer.Instance.GetHashCode((ArraySegment<byte>)obj.Key);
        }

        [DoesNotReturn]
        static void ThrowInvalidMapKey(Type type)
        {
            throw new ArgumentException(CommonResources.GetString(CommonResources.InvalidType, nameof(MapKey), nameof(ArraySegment<byte>), type));
        }
    }
}
