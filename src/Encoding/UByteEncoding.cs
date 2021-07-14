// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    sealed class UByteEncoding : PrimitiveEncoding<byte>
    {
        public UByteEncoding()
            : base(FormatCode.UByte)
        {
        }

        public static int GetEncodeSize(byte? value)
        {
            return value.HasValue ? FixedWidth.UByteEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(byte? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.UByte);
                AmqpBitConverter.WriteUByte(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static byte? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadUByte(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.UByte : UByteEncoding.GetEncodeSize((byte)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)value);
            }
            else
            {
                UByteEncoding.Encode((byte)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return UByteEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<byte> value)
        {
            return FixedWidth.UByte * value.Count;
        }

        public override void EncodeArray(IList<byte> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.UByte * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is byte[] byteArray)
            {
                // fast-path for byte[] so the bounds checks can be elided
                byteArray.AsSpan().CopyTo(destination);
            }
            else
            {
                IReadOnlyList<byte> listValue = (IReadOnlyList<byte>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    destination.Slice(FixedWidth.UByte * i)[0] = listValue[i];
                }
            }

            buffer.Append(byteCount);
        }

        public override byte[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.UByte * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            byte[] array = new byte[count];
            source.CopyTo(array);
            buffer.Complete(byteCount);
            return array;
        }
    }
}
