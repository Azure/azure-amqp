// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class UShortEncoding : EncodingBase
    {
        public UShortEncoding()
            : base(FormatCode.UShort)
        {
        }

        public static int GetEncodeSize(ushort? value)
        {
            return value.HasValue ? FixedWidth.UShortEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(ushort? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.UShort);
                AmqpBitConverter.WriteUShort(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static ushort? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadUShort(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.UShort;
            }
            else
            {
                return UShortEncoding.GetEncodeSize((ushort)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteUShort(buffer, (ushort)value);
            }
            else
            {
                UShortEncoding.Encode((ushort)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return UShortEncoding.Decode(buffer, formatCode);
        }
    }
}
