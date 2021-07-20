// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class IntEncoding : EncodingBase<int>
    {
        public IntEncoding()
            : base(FormatCode.Int)
        {
        }

        public static int GetEncodeSize(int value)
        {
            return value < sbyte.MinValue || value > sbyte.MaxValue ? FixedWidth.IntEncoded : FixedWidth.ByteEncoded;
        }

        public static void Encode(int value, ByteBuffer buffer)
        {
            if (value < sbyte.MinValue || value > sbyte.MaxValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Int);
                AmqpBitConverter.WriteInt(buffer, value);
            }
            else
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallInt);
                AmqpBitConverter.WriteByte(buffer, (sbyte)value);
            }
        }

        public static int Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == FormatCode.SmallInt)
            {
                return AmqpBitConverter.ReadByte(buffer);
            }
            else if (formatCode == FormatCode.Int)
            {
                return AmqpBitConverter.ReadInt(buffer);
            }

            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
        }

        public override int GetArrayValueSize(int[] array)
        {
            return FixedWidth.Int * array.Length;
        }

        public override void WriteArrayValue(int[] array, ByteBuffer buffer)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0; i < array.Length; i++)
            {
                AmqpBitConverter.WriteUInt(buffer.Buffer, buffer.WritePos + i * FixedWidth.Int, (uint)array[i]);
            }

            buffer.Append(size);
        }

        public override int[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, int[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = Decode(buffer, formatCode);
            }

            return array;
        }

        protected override int OnGetSize(int value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Int;
        }

        protected override void OnWrite(int value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteInt(buffer, value);
            }
        }

        protected override int OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
