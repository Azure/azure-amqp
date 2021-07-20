// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using UTF32Encoding = System.Text.UTF32Encoding;

    sealed class CharEncoding : EncodingBase<char>
    {
        static readonly UTF32Encoding AmqpUTF32 = new UTF32Encoding(true, false);

        public CharEncoding()
            : base(FormatCode.Char, FixedWidth.Char)
        {
        }

        public static int GetEncodeSize(char value)
        {
            return FixedWidth.CharEncoded;
        }

        public static void Encode(char value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Char);
            FastEncode(value, buffer);
        }

        public static char Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return FastDecode(buffer, formatCode);
        }

        public override int GetArrayValueSize(char[] array)
        {
            return FixedWidth.Char * array.Length;
        }

        public override void WriteArrayValue(char[] array, ByteBuffer buffer)
        {
            int size = FixedWidth.Char * array.Length;
            buffer.ValidateWrite(size);
            AmqpUTF32.GetBytes(array, 0, array.Length, buffer.Buffer, buffer.WritePos);
            buffer.Append(size);
        }

        public override char[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, char[] array)
        {
            int size = FixedWidth.Char * array.Length;
            buffer.ValidateRead(size);
            AmqpUTF32.GetChars(buffer.Buffer, buffer.Offset, size, array, 0);
            buffer.Complete(size);
            return array;
        }

        protected override int OnGetSize(char value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.CharEncoded : FixedWidth.Char;
        }

        protected override void OnWrite(char value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                FastEncode(value, buffer);
            }
        }

        protected override char OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return FastDecode(buffer, formatCode);
        }

        static unsafe void FastEncode(char value, ByteBuffer buffer)
        {
            buffer.ValidateWrite(FixedWidth.Char);
            char* array = stackalloc char[1];
            array[0] = value;
            fixed (byte* ptr = &buffer.Buffer[buffer.WritePos])
            {
                AmqpUTF32.GetBytes(array, 1, ptr, FixedWidth.Char);
            }

            buffer.Append(FixedWidth.Char);
        }

        static unsafe char FastDecode(ByteBuffer buffer, FormatCode formatCode)
        {
            buffer.ValidateRead(FixedWidth.Char);
            char* array = stackalloc char[1];
            fixed (byte* ptr = &buffer.Buffer[buffer.Offset])
            {
                AmqpUTF32.GetChars(ptr, FixedWidth.Char, array, 1);
            }

            buffer.Complete(FixedWidth.Char);
            return array[0];
        }
    }
}
