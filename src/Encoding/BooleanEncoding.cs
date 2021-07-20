// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class BooleanEncoding : EncodingBase<bool>
    {
        public BooleanEncoding()
            : base(FormatCode.Boolean)
        {
        }

        public static int GetEncodeSize(bool value)
        {
            return FixedWidth.BooleanEncoded;
        }

        public static void Encode(bool value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, value ? FormatCode.BooleanTrue : FormatCode.BooleanFalse);
        }

        public static bool Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == FormatCode.Boolean)
            {
                return AmqpBitConverter.ReadUByte(buffer) == (byte)1;
            }

            return formatCode == FormatCode.BooleanTrue;
        }

        public override int GetArrayValueSize(bool[] array)
        {
            return array.Length;
        }

        public override void WriteArrayValue(bool[] array, ByteBuffer buffer)
        {
            buffer.ValidateWrite(array.Length);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos++)
            {
                buffer.Buffer[pos] = (byte)(array[i] ? 1 : 0);
            }

            buffer.Append(array.Length);
        }

        public override bool[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, bool[] array)
        {
            buffer.ValidateRead(array.Length);
            for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos++)
            {
                array[i] = buffer.Buffer[pos] == 1;
            }

            buffer.Append(array.Length);
            return array;
        }

        protected override int OnGetSize(bool value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.BooleanEncoded : FixedWidth.BooleanVarEncoded;
        }

        protected override void OnWrite(bool value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)(value ? 1 : 0));
            }
        }

        protected override bool OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
