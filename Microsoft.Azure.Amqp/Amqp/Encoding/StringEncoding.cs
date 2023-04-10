// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
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
                int stringSize = Encoding.UTF8.GetByteCount(value);
                int encodeWidth = AmqpEncoding.GetEncodeWidthBySize(stringSize);
                if (encodeWidth == FixedWidth.UByte)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.String8Utf8);
                    AmqpBitConverter.WriteUByte(buffer, (byte)stringSize);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.String32Utf8);
                    AmqpBitConverter.WriteUInt(buffer, (uint)stringSize);
                }

                buffer.Validate(true, stringSize);
                int bytes = Encoding.UTF8.GetBytes(value, 0, value.Length, buffer.Buffer, buffer.WritePos);
                Fx.Assert(bytes == stringSize, "size wrong");
                buffer.Append(stringSize);
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
                count = (int)AmqpBitConverter.ReadUByte(buffer);
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
            else
            {
                return StringEncoding.GetEncodeSize((string)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                string strValue = (string)value;
                int stringSize = Encoding.UTF8.GetByteCount(strValue);
                AmqpBitConverter.WriteUInt(buffer, (uint)stringSize);

                buffer.Validate(true, stringSize);
                int bytes = Encoding.UTF8.GetBytes(strValue, 0, strValue.Length, buffer.Buffer, buffer.WritePos);
                Fx.Assert(bytes == stringSize, "size wrong");
                buffer.Append(stringSize);
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
    }
}
