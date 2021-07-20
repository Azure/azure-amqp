// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System.Diagnostics;
    using System.Text;

    sealed class StringEncoding : EncodingBase<string>
    {
        public StringEncoding()
            : base(FormatCode.String32Utf8)
        {
        }

        public static int GetEncodeSize(string value)
        {
            int byteCount = Encoding.UTF8.GetByteCount(value);
            return FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(byteCount) + byteCount;
        }

        public static void Encode(string value, ByteBuffer buffer)
        {
            int byteCount = Encoding.UTF8.GetByteCount(value);
            if (byteCount <= byte.MaxValue)
            {
                AmqpBitConverter.Write(buffer, FormatCode.String8Utf8, (byte)byteCount);
            }
            else
            {
                AmqpBitConverter.Write(buffer, FormatCode.String32Utf8, (uint)byteCount);
            }

            buffer.ValidateWrite(byteCount);
            int encodedByteCount = Encoding.UTF8.GetBytes(value, 0, value.Length, buffer.Buffer, buffer.WritePos);
            Debug.Assert(byteCount == encodedByteCount);
            buffer.Append(encodedByteCount);
        }

        public static string Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            int length;
            if (formatCode == FormatCode.String8Utf8)
            {
                length = (int)AmqpBitConverter.ReadUByte(buffer);
            }
            else if (formatCode == FormatCode.String32Utf8)
            {
                length = AmqpBitConverter.ReadInt(buffer);
            }
            else
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            buffer.ValidateRead(length);
            string value = Encoding.UTF8.GetString(buffer.Buffer, buffer.Offset, length);
            buffer.Complete(length);
            return value;
        }

        public override int GetArrayValueSize(string[] array)
        {
            int size = 0;
            for (int i = 0; i < array.Length; i++)
            {
                size += this.OnGetSize(array[i], i);
            }

            return size;
        }

        public override void WriteArrayValue(string[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                this.OnWrite(array[i], buffer, i);
            }
        }

        public override string[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, string[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = Decode(buffer, formatCode);
            }

            return array;
        }

        protected override int OnGetSize(string value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Int + Encoding.UTF8.GetByteCount(value);
        }

        protected override void OnWrite(string value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                int byteCount = Encoding.UTF8.GetByteCount(value);
                AmqpBitConverter.WriteInt(buffer, byteCount);
                buffer.ValidateWrite(byteCount);
                int count = Encoding.UTF8.GetBytes(value, 0, value.Length, buffer.Buffer, buffer.WritePos);
                buffer.Append(count);
            }
        }

        protected override string OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
