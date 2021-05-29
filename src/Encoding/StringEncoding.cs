// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Text;

    sealed class StringEncoding : EncodingBase
    {
        public StringEncoding()
            : base(FormatCode.String32Utf8)
        {
        }

        public static int GetEncodeSize(string value)
        {
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }

            int stringSize = Encoding.UTF8.GetByteCount(value);
            return FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(stringSize) + stringSize;
        }

        public static void Encode(string value, ByteBuffer buffer)
        {
            if (value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ReadOnlySpan<byte> encodedData = Encoding.UTF8.GetBytes(value);
                int encodeWidth = AmqpEncoding.GetEncodeWidthBySize(encodedData.Length);
                AmqpBitConverter.WriteUByte(buffer, encodeWidth == FixedWidth.UByte ? FormatCode.String8Utf8 : FormatCode.String32Utf8);
                StringEncoding.Encode(encodedData, encodeWidth, buffer);
            }
        }

        public static string Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            int count;
            Encoding encoding;

            if (formatCode == FormatCode.String8Utf8)
            {
                count = AmqpBitConverter.ReadUByte(buffer);
                encoding = Encoding.UTF8;
            }
            else if (formatCode == FormatCode.String32Utf8)
            {
                count = (int)AmqpBitConverter.ReadUInt(buffer);
                encoding = Encoding.UTF8;
            }
            else
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            string value = encoding.GetString(buffer.Buffer, buffer.Offset, count);
            buffer.Complete(count);

            return value;
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.UInt + Encoding.UTF8.GetByteCount((string)value);
            }

            return StringEncoding.GetEncodeSize((string)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                StringEncoding.Encode(Encoding.UTF8.GetBytes((string)value), FixedWidth.UInt, buffer);
            }
            else
            {
                StringEncoding.Encode((string)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return StringEncoding.Decode(buffer, formatCode);
        }

        static void Encode(ReadOnlySpan<byte> encodedData, int width, ByteBuffer buffer)
        {
            if (width == FixedWidth.UByte)
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)encodedData.Length);
            }
            else
            {
                AmqpBitConverter.WriteUInt(buffer, (uint)encodedData.Length);
            }

            AmqpBitConverter.WriteBytes(buffer, encodedData, 0, encodedData.Length);
        }
    }
}
