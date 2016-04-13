// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class FloatEncoding : EncodingBase
    {
        public FloatEncoding()
            : base(FormatCode.Float)
        {
        }

        public static int GetEncodeSize(float? value)
        {
            return value.HasValue ? FixedWidth.FloatEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(float? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Float);
                AmqpBitConverter.WriteFloat(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static float? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadFloat(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.Float;
            }
            else
            {
                return FloatEncoding.GetEncodeSize((float)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteFloat(buffer, (float)value);
            }
            else
            {
                FloatEncoding.Encode((float)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return FloatEncoding.Decode(buffer, formatCode);
        }
    }
}
