// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class ShortEncoding : PrimitiveEncoding<short>
    {
        public ShortEncoding()
            : base(FormatCode.Short)
        {
        }

        public static int GetEncodeSize(short? value)
        {
            return value.HasValue ? FixedWidth.ShortEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(short? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Short);
                AmqpBitConverter.WriteShort(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static short? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadShort(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.Short : ShortEncoding.GetEncodeSize((short)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteShort(buffer, (short)value);
            }
            else
            {
                ShortEncoding.Encode((short)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return ShortEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<short> value)
        {
            return FixedWidth.Short * value.Count;
        }

        public override void EncodeArray(IList<short> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Short * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is short[] shortArray)
            {
                // fast-path for ushort[] so the bounds checks can be elided
                for (int i = 0; i < shortArray.Length; i++)
                {
                    BinaryPrimitives.WriteInt16BigEndian(destination.Slice(FixedWidth.Short * i), shortArray[i]);
                }
            }
            else
            {
                IReadOnlyList<short> listValue = (IReadOnlyList<short>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteInt16BigEndian(destination.Slice(FixedWidth.Short * i), listValue[i]);
                }
            }

            buffer.Append(byteCount);
        }

        public override short[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.UShort * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            short[] array = new short[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BinaryPrimitives.ReadInt16BigEndian(source.Slice(FixedWidth.Short * i));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
