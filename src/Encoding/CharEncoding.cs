// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class CharEncoding : PrimitiveEncoding<char>
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
            return arrayEncoding ? FixedWidth.Char : CharEncoding.GetEncodeSize((char)value);
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

        public override int GetArrayEncodeSize(IList<char> value)
        {
            return FixedWidth.Char * value.Count;
        }

        public override void EncodeArray(IList<char> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Char * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is char[] charArray)
            {
                // fast-path for char[] so the bounds checks can be elided
                for (int i = 0; i < charArray.Length; i++)
                {
                    // There is probably a more efficient way but let's go with this one for now
                    BinaryPrimitives.WriteInt32BigEndian(destination.Slice(FixedWidth.Char * i), char.ConvertToUtf32(new string(charArray[i], 1), 0));
                }
            }
            else
            {
                IReadOnlyList<char> listValue = (IReadOnlyList<char>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    // There is probably a more efficient way but let's go with this one for now
                    BinaryPrimitives.WriteInt32BigEndian(destination.Slice(FixedWidth.Char * i), char.ConvertToUtf32(new string(listValue[i], 1), 0));
                }
            }

            buffer.Append(byteCount);
        }

        public override char[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.Char * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            char[] array = new char[count];
            for (int i = 0; i < count; ++i)
            {
                // There is probably a more efficient way but let's go with this one for now
                array[i] = char.ConvertFromUtf32(BinaryPrimitives.ReadInt32BigEndian(source.Slice(FixedWidth.Char * i)))[0];
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
