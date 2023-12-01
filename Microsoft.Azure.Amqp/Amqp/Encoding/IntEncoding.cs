// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class IntEncoding : EncodingBase
    {
        public IntEncoding()
            : base(FormatCode.Int)
        {
        }

        public static int GetEncodeSize(int? value)
        {
            if (value.HasValue)
            {
                return value.Value < sbyte.MinValue || value.Value > sbyte.MaxValue ?
                    FixedWidth.IntEncoded :
                    FixedWidth.ByteEncoded;
            }
            else
            {
                return FixedWidth.NullEncoded;
            }
        }

        public static void Encode(int? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                if (value < sbyte.MinValue || value > sbyte.MaxValue)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Int);
                    AmqpBitConverter.WriteInt(buffer, value.Value);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallInt);
                    AmqpBitConverter.WriteByte(buffer, (sbyte)value.Value);
                }
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static int? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Int, FormatCode.SmallInt);
            return formatCode == FormatCode.SmallInt ?
                AmqpBitConverter.ReadByte(buffer) :
                AmqpBitConverter.ReadInt(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.Int;
            }
            else
            {
                return IntEncoding.GetEncodeSize((int)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteInt(buffer, (int)value);
            }
            else
            {
                IntEncoding.Encode((int)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            int? value = IntEncoding.Decode(buffer, formatCode);
            if (value == null)
            {
                return null;
            }

            return EncodingCache.Box(value.Value);
        }
    }
}
