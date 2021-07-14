// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    sealed class ByteEncoding : PrimitiveEncoding<sbyte>
    {
        public ByteEncoding()
            : base(FormatCode.Byte)
        {
        }

        public static int GetEncodeSize(sbyte? value)
        {
            return value.HasValue ? FixedWidth.ByteEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(sbyte? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Byte);
                AmqpBitConverter.WriteByte(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static sbyte? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadByte(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.Byte : ByteEncoding.GetEncodeSize((sbyte)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteByte(buffer, (sbyte)value);
            }
            else
            {
                ByteEncoding.Encode((sbyte)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return ByteEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<sbyte> value)
        {
            return FixedWidth.Byte * value.Count;
        }

        public override void EncodeArray(IList<sbyte> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Byte * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is sbyte[] byteArray)
            {
                // fast-path for byte[] so the bounds checks can be elided
                for (int i = 0; i < byteArray.Length; i++)
                {
                    destination.Slice(FixedWidth.Byte * i)[0] = (byte) byteArray[i];
                }
            }
            else
            {
                IReadOnlyList<sbyte> listValue = (IReadOnlyList<sbyte>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    destination.Slice(FixedWidth.Byte * i)[0] = (byte) listValue[i];
                }
            }

            buffer.Append(byteCount);
        }

        public override sbyte[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.Byte * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            sbyte[] array = new sbyte[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = (sbyte) source.Slice(FixedWidth.Byte * i)[0];
            }
            buffer.Complete(byteCount);
            return array;
        }
    }
}
