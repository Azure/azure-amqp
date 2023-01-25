// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System.Collections.Generic;

    sealed class MapEncoding : EncodingBase<AmqpMap>
    {
        public MapEncoding()
            : base(FormatCode.Map32)
        {
        }

        public static void ReadMapValue(ByteBuffer buffer, AmqpMap map, int size, int count)
        {
            for (; count > 0; count -= 2)
            {
                object key = AmqpEncoding.DecodeObject(buffer);
                object item = AmqpCodec.DecodeObject(buffer);
                map[new MapKey(key)] = item;
            }
        }

        public static int GetEncodeSize(AmqpMap value)
        {
            // Return the max possible size because Encode needs the space to avoid calling GetEncodeSize again.
            int size = GetMapSize(value);
            return FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int + size;
        }

        public static void Encode(AmqpMap value, ByteBuffer buffer)
        {
            int encodeCount = value.Count * 2;
            var tracker = SizeTracker.Track(buffer);
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Map32);
            AmqpBitConverter.WriteInt(buffer, FixedWidth.Int);
            AmqpBitConverter.WriteInt(buffer, encodeCount);
            if (encodeCount > 0)
            {
                foreach (KeyValuePair<MapKey, object> item in value)
                {
                    AmqpEncoding.EncodeObject(item.Key.Key, buffer);
                    AmqpEncoding.EncodeObject(item.Value, buffer);
                }

                // compact if necessary
                int size = tracker.Length - 9; // FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int;
                if (size < byte.MaxValue && encodeCount <= byte.MaxValue)
                {
                    tracker.Compact(FormatCode.Map8, (byte)(size + 1), (byte)encodeCount, 9);
                }
                else
                {
                    tracker.CommitExclusive(FixedWidth.FormatCode);
                }
            }
        }

        public static AmqpMap Decode(ByteBuffer buffer, FormatCode formatCode, IEqualityComparer<MapKey> comparer = null)
        {
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Map8, FormatCode.Map32, out int size, out int count);
            AmqpMap map = comparer != null ? new AmqpMap(comparer) : new AmqpMap();
            ReadMapValue(buffer, map, size, count);
            return map;
        }

        protected override int OnGetSize(AmqpMap value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Int + FixedWidth.Int + GetMapSize(value);
        }

        protected override void OnWrite(AmqpMap value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                EncodeArrayItem(value, arrayIndex, buffer);
            }
        }

        protected override AmqpMap OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }

        static int GetMapSize(AmqpMap value)
        {
            int size = 0;
            if (value.Count > 0)
            {
                foreach (KeyValuePair<MapKey, object> item in value)
                {
                    size += AmqpEncoding.GetObjectEncodeSize(item.Key.Key);
                    size += AmqpEncoding.GetObjectEncodeSize(item.Value);
                }
            }

            return size;
        }

        static void EncodeArrayItem(AmqpMap value, int index, ByteBuffer buffer)
        {
            var tracker = SizeTracker.Track(buffer);
            AmqpBitConverter.WriteInt(buffer, FixedWidth.Int);
            AmqpBitConverter.WriteInt(buffer, value.Count);
            if (value.Count > 0)
            {
                foreach (object item in value)
                {
                    AmqpEncoding.EncodeObject(item, buffer);
                }

                tracker.CommitExclusive(0);
            }
        }
    }
}
