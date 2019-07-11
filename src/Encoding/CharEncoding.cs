// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class CharEncoding : EncodingBase
    {
        public CharEncoding()
            : base(FormatCode.Char)
        {
        }

        public static int GetEncodeSize(char? value)
        {
            return value.HasValue ? FixedWidth.CharEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(char? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Char);
                AmqpBitConverter.WriteInt(buffer, char.ConvertToUtf32(new string(value.Value, 1), 0));
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static char? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            int intValue = AmqpBitConverter.ReadInt(buffer);
            string value = char.ConvertFromUtf32(intValue);
            if (value.Length > 1)
            {
                throw new ArgumentOutOfRangeException(CommonResources.ErrorConvertingToChar);
            }

            return value[0];
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.Char;
            }
            else
            {
                return CharEncoding.GetEncodeSize((char)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteInt(buffer, char.ConvertToUtf32(new string((char)value, 1), 0));
            }
            else
            {
                CharEncoding.Encode((char)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return CharEncoding.Decode(buffer, formatCode);
        }
    }
}
