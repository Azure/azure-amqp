// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class LongEncoding : PrimitiveEncoding<long>
    {
        public LongEncoding()
            : base(FormatCode.Long)
        {
        }

        public static int GetEncodeSize(long? value)
        {
            if (value.HasValue)
            {
                return value < sbyte.MinValue || value > sbyte.MaxValue ?
                    FixedWidth.LongEncoded :
                    FixedWidth.UByteEncoded;
            }

            return FixedWidth.NullEncoded;
        }

        public static void Encode(long? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                if (value < sbyte.MinValue || value > sbyte.MaxValue)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Long);
                    AmqpBitConverter.WriteLong(buffer, value.Value);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallLong);
                    AmqpBitConverter.WriteByte(buffer, (sbyte)value);
                }
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static long? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Long, FormatCode.SmallLong);
            return formatCode == FormatCode.SmallLong ?
                AmqpBitConverter.ReadByte(buffer) :
                AmqpBitConverter.ReadLong(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.Long : LongEncoding.GetEncodeSize((long)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteLong(buffer, (long)value);
            }
            else
            {
                LongEncoding.Encode((long)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return LongEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<long> value)
        {
            return FixedWidth.Long * value.Count;
        }

        public override void EncodeArray(IList<long> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Long * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is long[] longArray)
            {
                // fast-path for long[] so the bounds checks can be elided
                for (int i = 0; i < longArray.Length; i++)
                {
                    BinaryPrimitives.WriteInt64BigEndian(destination.Slice(FixedWidth.Long * i), longArray[i]);
                }
            }
            else
            {
                IReadOnlyList<long> listValue = (IReadOnlyList<long>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteInt64BigEndian(destination.Slice(FixedWidth.Long * i), listValue[i]);
                }
            }

            buffer.Append(byteCount);
        }

        public override long[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.Long * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            long[] array = new long[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BinaryPrimitives.ReadInt64BigEndian(source.Slice(FixedWidth.Long * i));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
