// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class UShortEncoding : PrimitiveEncoding<ushort>
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
            return arrayEncoding ? FixedWidth.UShort : UShortEncoding.GetEncodeSize((ushort)value);
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

        public override int GetArrayEncodeSize(IList<ushort> value)
        {
            return FixedWidth.UShort * value.Count;
        }

        public override void EncodeArray(IList<ushort> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.UShort * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is ushort[] ushortArray)
            {
                // fast-path for ushort[] so the bounds checks can be elided
                for (int i = 0; i < ushortArray.Length; i++)
                {
                    BinaryPrimitives.WriteUInt16BigEndian(destination.Slice(FixedWidth.UShort * i), ushortArray[i]);
                }
            }
            else
            {
                IReadOnlyList<ushort> listValue = (IReadOnlyList<ushort>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteUInt16BigEndian(destination.Slice(FixedWidth.UShort * i), listValue[i]);
                }
            }

            buffer.Append(byteCount);
        }

        public override ushort[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.UShort * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            ushort[] array = new ushort[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BinaryPrimitives.ReadUInt16BigEndian(source.Slice(FixedWidth.UShort * i));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
