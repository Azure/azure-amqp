// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class LongEncoding : EncodingBase<long>
    {
        public LongEncoding()
            : base(FormatCode.Long)
        {
        }

        public static int GetEncodeSize(long value)
        {
            return value < sbyte.MinValue || value > sbyte.MaxValue ? FixedWidth.LongEncoded : FixedWidth.ByteEncoded;
        }

        public static void Encode(long value, ByteBuffer buffer)
        {
            if (value < sbyte.MinValue || value > sbyte.MaxValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Long);
                AmqpBitConverter.WriteLong(buffer, value);
            }
            else
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallLong);
                AmqpBitConverter.WriteByte(buffer, (sbyte)value);
            }
        }

        public static long Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == FormatCode.SmallLong)
            {
                return AmqpBitConverter.ReadByte(buffer);
            }
            else if (formatCode == FormatCode.Long)
            {
                return AmqpBitConverter.ReadLong(buffer);
            }

            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
        }

        public override int GetArrayValueSize(long[] array)
        {
            return FixedWidth.Long * array.Length;
        }

        public override void WriteArrayValue(long[] array, ByteBuffer buffer)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.Long)
            {
                AmqpBitConverter.WriteULong(buffer.Buffer, pos, (ulong)array[i]);
            }

            buffer.Append(size);
        }

        public override long[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, long[] array)
        {
            AmqpEncoding.VerifyFormatCode(formatCode, buffer.Offset, FormatCode.SmallLong, FormatCode.Long);
            int size;
            if (formatCode == FormatCode.SmallLong)
            {
                size = array.Length;
                buffer.ValidateRead(size);
                for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos++)
                {
                    array[i] = buffer.Buffer[pos];
                }
            }
            else
            {
                size = FixedWidth.Long * array.Length;
                buffer.ValidateRead(size);
                for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.Long)
                {
                    array[i] = (long)AmqpBitConverter.ReadULong(buffer.Buffer, pos, FixedWidth.ULong);
                }
            }

            buffer.Complete(size);
            return array;
        }

        protected override int OnGetSize(long value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Long;
        }

        protected override void OnWrite(long value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteLong(buffer, value);
            }
        }

        protected override long OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
