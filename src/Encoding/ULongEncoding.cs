// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class ULongEncoding : EncodingBase<ulong>
    {
        public ULongEncoding()
            : base(FormatCode.ULong)
        {
        }

        public static int GetEncodeSize(ulong value)
        {
            return value == 0ul ? FixedWidth.Zero : (value <= byte.MaxValue ? FixedWidth.UByteEncoded : FixedWidth.ULongEncoded);
        }

        public static void Encode(ulong value, ByteBuffer buffer)
        {
            if (value == 0ul)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.ULong0);
            }
            else if (value <= byte.MaxValue)
            {
                AmqpBitConverter.Write(buffer, FormatCode.SmallULong, (byte)value);
            }
            else
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.ULong);
                AmqpBitConverter.WriteULong(buffer, value);
            }
        }

        public static ulong Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == FormatCode.ULong0)
            {
                return 0ul;
            }
            else if (formatCode == FormatCode.SmallULong)
            {
                return AmqpBitConverter.ReadUByte(buffer);
            }
            else if (formatCode == FormatCode.ULong)
            {
                return AmqpBitConverter.ReadULong(buffer);
            }

            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
        }

        public override int GetArrayValueSize(ulong[] array)
        {
            return FixedWidth.ULong * array.Length;
        }

        public override void WriteArrayValue(ulong[] array, ByteBuffer buffer)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.ULong)
            {
                AmqpBitConverter.WriteULong(buffer.Buffer, pos, array[i]);
            }

            buffer.Append(size);
        }

        public override ulong[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, ulong[] array)
        {
            if (formatCode == FormatCode.ULong0)
            {
                return array;
            }

            AmqpEncoding.VerifyFormatCode(formatCode, buffer.Offset, FormatCode.SmallULong, FormatCode.ULong);
            int size;
            if (formatCode == FormatCode.SmallULong)
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
                size = FixedWidth.ULong * array.Length;
                buffer.ValidateRead(size);
                for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.ULong)
                {
                    array[i] = AmqpBitConverter.ReadULong(buffer.Buffer, pos, FixedWidth.ULong);
                }
            }

            buffer.Complete(size);
            return array;
        }

        protected override int OnGetSize(ulong value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.ULong;
        }

        protected override void OnWrite(ulong value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteULong(buffer, value);
            }
        }

        protected override ulong OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
