// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class UIntEncoding : PrimitiveEncoding<uint>
    {
        public UIntEncoding()
            : base(FormatCode.UInt)
        {
        }

        public static int GetEncodeSize(uint? value)
        {
            if (value.HasValue)
            {
                if (value.Value == 0)
                {
                    return FixedWidth.ZeroEncoded;
                }

                return value.Value <= byte.MaxValue ? FixedWidth.UByteEncoded : FixedWidth.UIntEncoded;
            }

            return FixedWidth.NullEncoded;
        }

        public static void Encode(uint? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                if (value == 0)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.UInt0);
                }
                else if (value.Value <= byte.MaxValue)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.SmallUInt);
                    AmqpBitConverter.WriteUByte(buffer, (byte)value.Value);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.UInt);
                    AmqpBitConverter.WriteUInt(buffer, value.Value);
                }
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static uint? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            VerifyFormatCode(formatCode, buffer.Offset, FormatCode.UInt, FormatCode.SmallUInt, FormatCode.UInt0);
            if (formatCode == FormatCode.UInt0)
            {
                return 0;
            }

            return formatCode == FormatCode.SmallUInt ?
                AmqpBitConverter.ReadUByte(buffer) :
                AmqpBitConverter.ReadUInt(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.UInt : UIntEncoding.GetEncodeSize((uint)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteUInt(buffer, (uint)value);
            }
            else
            {
                UIntEncoding.Encode((uint)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return UIntEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<uint> value)
        {
            return FixedWidth.UInt * value.Count;
        }

        public override void EncodeArray(IList<uint> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.UInt * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is uint[] intArray)
            {
                // fast-path for int[] so the bounds checks can be elided
                for (int i = 0; i < intArray.Length; i++)
                {
                    BinaryPrimitives.WriteUInt32BigEndian(destination.Slice(FixedWidth.UInt * i), intArray[i]);
                }
            }
            else
            {
                IReadOnlyList<uint> listValue = (IReadOnlyList<uint>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteUInt32BigEndian(destination.Slice(FixedWidth.UInt * i), listValue[i]);
                }
            }

            buffer.Append(byteCount);
        }

        public override uint[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.UInt * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            uint[] array = new uint[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BinaryPrimitives.ReadUInt32BigEndian(source.Slice(FixedWidth.UInt * i));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
