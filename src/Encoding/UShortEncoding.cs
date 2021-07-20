// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class UShortEncoding : EncodingBase<ushort>
    {
        public UShortEncoding()
            : base(FormatCode.UShort)
        {
        }

        public static int GetEncodeSize(ushort value)
        {
            return FixedWidth.UShortEncoded;
        }

        public static void Encode(ushort value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.UShort);
            AmqpBitConverter.WriteUShort(buffer, value);
        }

        public static ushort Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadUShort(buffer);
        }

        public override int GetArrayValueSize(ushort[] array)
        {
            return FixedWidth.UShort * array.Length;
        }

        public override void WriteArrayValue(ushort[] array, ByteBuffer buffer)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.UShort)
            {
                AmqpBitConverter.WriteUShort(buffer.Buffer, pos, array[i]);
            }

            buffer.Append(size);
        }

        public override ushort[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, ushort[] array)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateRead(size);
            for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.UShort)
            {
                array[i] = AmqpBitConverter.ReadUShort(buffer.Buffer, pos, FixedWidth.UShort);
            }

            buffer.Complete(size);
            return array;
        }

        protected override int OnGetSize(ushort value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.UShort;
        }

        protected override void OnWrite(ushort value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteUShort(buffer, value);
            }
        }

        protected override ushort OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
