// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections;
    using System.Collections.Generic;

    sealed class IntEncoding : PrimitiveEncoding
    {
        public IntEncoding()
            : base(FormatCode.Int)
        {
        }

        public static int GetEncodeSize(int? value)
        {
            if (value.HasValue)
            {
                return value.Value < sbyte.MinValue || value.Value > sbyte.MaxValue ?
                    FixedWidth.IntEncoded :
                    FixedWidth.ByteEncoded;
            }
            else
            {
                return FixedWidth.NullEncoded;
            }
        }

        public static void Encode(int? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                if (value < sbyte.MinValue || value > sbyte.MaxValue)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Int);
                    AmqpBitConverter.WriteInt(buffer, value.Value);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallInt);
                    AmqpBitConverter.WriteByte(buffer, (sbyte)value.Value);
                }
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static int? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Int, FormatCode.SmallInt);
            return formatCode == FormatCode.SmallInt ?
                AmqpBitConverter.ReadByte(buffer) :
                AmqpBitConverter.ReadInt(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.Int;
            }
            else
            {
                return IntEncoding.GetEncodeSize((int)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteInt(buffer, (int)value);
            }
            else
            {
                IntEncoding.Encode((int)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return IntEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList value)
        {
            return FixedWidth.Int * value.Count;
        }

        public override void EncodeArray(IList value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Int * value.Count;

            buffer.Validate(write: true, byteCount);
            Span<byte> destination = buffer.GetWriteSpan();
            
            if (value is int[] intArray)
            {
                // fast-path for int[] so the bounds checks can be elided
                for (int i = 0; i < intArray.Length; i++)
                {
                    BinaryPrimitives.WriteInt32BigEndian(destination.Slice(FixedWidth.Int * i), intArray[i]);
                }
            }
            else
            {
                IReadOnlyList<int> listValue = (IReadOnlyList<int>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteInt32BigEndian(destination.Slice(FixedWidth.Int * i), listValue[i]);
                }
            }

            buffer.Append(byteCount);
        }

        public override Array DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.Int * count;

            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();
            
            int[] array = new int[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BinaryPrimitives.ReadInt32BigEndian(source.Slice(FixedWidth.Int * i));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
