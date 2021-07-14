// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class ULongEncoding : PrimitiveEncoding<ulong>
    {
        public ULongEncoding()
            : base(FormatCode.ULong)
        {
        }

        public static int GetEncodeSize(ulong? value)
        {
            if (value.HasValue)
            {
                if (value.Value == 0)
                {
                    return FixedWidth.ZeroEncoded;
                }

                return value.Value <= byte.MaxValue ? FixedWidth.UByteEncoded : FixedWidth.ULongEncoded;
            }

            return FixedWidth.NullEncoded;
        }

        public static void Encode(ulong? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                if (value == 0)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.ULong0);
                }
                else if (value <= byte.MaxValue)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallULong);
                    AmqpBitConverter.WriteUByte(buffer, (byte)value.Value);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.ULong);
                    AmqpBitConverter.WriteULong(buffer, value.Value);
                }
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static ulong? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            VerifyFormatCode(formatCode, buffer.Offset, FormatCode.ULong, FormatCode.SmallULong, FormatCode.ULong0);
            if (formatCode == FormatCode.ULong0)
            {
                return 0;
            }

            return formatCode == FormatCode.SmallULong ?
                AmqpBitConverter.ReadUByte(buffer) :
                AmqpBitConverter.ReadULong(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.ULong : ULongEncoding.GetEncodeSize((ulong)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteULong(buffer, (ulong)value);
            }
            else
            {
                ULongEncoding.Encode((ulong)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return ULongEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<ulong> value)
        {
            return FixedWidth.ULong * value.Count;
        }

        public override void EncodeArray(IList<ulong> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.ULong * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is ulong[] longArray)
            {
                // fast-path for long[] so the bounds checks can be elided
                for (int i = 0; i < longArray.Length; i++)
                {
                    BinaryPrimitives.WriteUInt64BigEndian(destination.Slice(FixedWidth.ULong * i), longArray[i]);
                }
            }
            else
            {
                IReadOnlyList<ulong> listValue = (IReadOnlyList<ulong>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteUInt64BigEndian(destination.Slice(FixedWidth.ULong * i), listValue[i]);
                }
            }

            buffer.Append(byteCount);
        }

        public override ulong[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.ULong * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            ulong[] array = new ulong[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BinaryPrimitives.ReadUInt64BigEndian(source.Slice(FixedWidth.ULong * i));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
