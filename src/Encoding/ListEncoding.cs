// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System.Collections;
    using System.Collections.Generic;

    sealed class ListEncoding : EncodingBase<IList>
    {
        public ListEncoding()
            : base(FormatCode.List32)
        {
        }

        public static int GetEncodeSize(IList value)
        {
            if (value.Count == 0)
            {
                return FixedWidth.FormatCode;
            }

            // Return the max possible size because Encode needs the space to avoid calling GetEncodeSize again.
            int size = GetListSize(value);
            return FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int + size;
        }

        public static void Encode(IList value, ByteBuffer buffer)
        {
            if (value.Count == 0)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.List0);
                return;
            }

            var tracker = SizeTracker.Track(buffer);
            AmqpBitConverter.WriteUByte(buffer, FormatCode.List32);
            AmqpBitConverter.WriteInt(buffer, FixedWidth.Int);
            AmqpBitConverter.WriteInt(buffer, value.Count);
            foreach (object item in value)
            {
                AmqpEncoding.EncodeObject(item, buffer);
            }

            // compact if necessary
            int size = tracker.Length - 9;
            if (size < byte.MaxValue && value.Count <= byte.MaxValue)
            {
                tracker.Compact(FormatCode.List8, (byte)(size + 1), (byte)value.Count, 9);
            }
            else
            {
                tracker.CommitExclusive(FixedWidth.FormatCode);
            }
        }

        public static IList Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == FormatCode.List0)
            {
                return new object[0];
            }

            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.List8, FormatCode.List32, out int size, out int count);
            List<object> list = new List<object>(count);
            for (int i = 0; i < count; i++)
            {
                list.Add(AmqpEncoding.DecodeObject(buffer));
            }

            return list;
        }

        protected override int OnGetSize(IList value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Int + FixedWidth.Int + GetListSize(value);
        }

        protected override void OnWrite(IList value, ByteBuffer buffer, int arrayIndex)
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

        protected override IList OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }

        static int GetListSize(IList value)
        {
            int size = 0;
            if (value.Count > 0)
            {
                foreach (object item in value)
                {
                    size += AmqpEncoding.GetObjectEncodeSize(item);
                }
            }

            return size;
        }

        static void EncodeArrayItem(IList value, int index, ByteBuffer buffer)
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
