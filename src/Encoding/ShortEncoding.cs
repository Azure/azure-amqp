// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class ShortEncoding : EncodingBase<short>
    {
        public ShortEncoding()
            : base(FormatCode.Short, FixedWidth.Short)
        {
        }

        public static int GetEncodeSize(short value)
        {
            return FixedWidth.ShortEncoded;
        }

        public static void Encode(short value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Short);
            AmqpBitConverter.WriteShort(buffer, value);
        }

        public static short Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadShort(buffer);
        }

        public override int GetArrayValueSize(short[] array)
        {
            return FixedWidth.Short * array.Length;
        }

        public override void WriteArrayValue(short[] array, ByteBuffer buffer)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.Short)
            {
                AmqpBitConverter.WriteUShort(buffer.Buffer, pos, (ushort)array[i]);
            }

            buffer.Append(size);
        }

        public override short[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, short[] array)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateRead(size);
            for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.Short)
            {
                array[i] = (short)AmqpBitConverter.ReadUShort(buffer.Buffer, pos, FixedWidth.UShort);
            }

            buffer.Complete(size);
            return array;
        }

        protected override int OnGetSize(short value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.ShortEncoded : FixedWidth.Short;
        }

        protected override void OnWrite(short value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteShort(buffer, value);
            }
        }

        protected override short OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
