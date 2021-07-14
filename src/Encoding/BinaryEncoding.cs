// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    sealed class BinaryEncoding : PrimitiveEncoding<ArraySegment<byte>>
    {
        public BinaryEncoding()
            : base(FormatCode.Binary32)
        {
        }

        public static int GetEncodeSize(ArraySegment<byte> value)
        {
            return value.Array == null ?
                FixedWidth.NullEncoded :
                FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(value.Count) + value.Count;
        }

        public static void Encode(ArraySegment<byte> value, ByteBuffer buffer)
        {
            if (value.Array == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                int width = AmqpEncoding.GetEncodeWidthBySize(value.Count);
                if (width == FixedWidth.UByte)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Binary8);
                    AmqpBitConverter.WriteUByte(buffer, (byte)value.Count);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Binary32);
                    AmqpBitConverter.WriteUInt(buffer, (uint)value.Count);
                }

                AmqpBitConverter.WriteBytes(buffer, value, value.Offset, value.Count);
            }
        }

        public static ArraySegment<byte> Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode, true);
        }

        public static ArraySegment<byte> Decode(ByteBuffer buffer, FormatCode formatCode, bool copy)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return AmqpConstants.NullBinary;
            }

            int count;
            AmqpEncoding.ReadCount(buffer, formatCode, FormatCode.Binary8, FormatCode.Binary32, out count);
            if (count == 0)
            {
                return AmqpConstants.EmptyBinary;
            }

            ArraySegment<byte> value;
            if (copy)
            {
                byte[] valueBuffer = new byte[count];
                Buffer.BlockCopy(buffer.Buffer, buffer.Offset, valueBuffer, 0, count);
                value = new ArraySegment<byte>(valueBuffer, 0, count);
            }
            else
            {
                value = new ArraySegment<byte>(buffer.Buffer, buffer.Offset, count);
            }

            buffer.Complete(count);
            return value;
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.UInt + ((ArraySegment<byte>)value).Count;
            }

            return BinaryEncoding.GetEncodeSize((ArraySegment<byte>)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                ArraySegment<byte> binaryValue = (ArraySegment<byte>)value;
                AmqpBitConverter.WriteUInt(buffer, (uint)binaryValue.Count);
                AmqpBitConverter.WriteBytes(buffer, binaryValue, binaryValue.Offset, binaryValue.Count);
            }
            else
            {
                BinaryEncoding.Encode((ArraySegment<byte>)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return BinaryEncoding.Decode(buffer, formatCode, false);
        }

        public override int GetArrayEncodeSize(IList<ArraySegment<byte>> value)
        {
            int size = 0;
            for (int i = 0; i < value.Count; i++)
            {
                size += AmqpEncoding.GetEncodeWidthBySize(value[i].Count);
            }
            return size;
        }

        public override void EncodeArray(IList<ArraySegment<byte>> value, ByteBuffer buffer)
        {
            if (value is ArraySegment<byte>[] arraySegments)
            {
                // fast-path for ArraySegment<byte>[] so the bounds checks can be elided
                for (int i = 0; i < arraySegments.Length; i++)
                {
                    var segment = arraySegments[i];
                    AmqpBitConverter.WriteUInt(buffer, (uint)segment.Count);
                    AmqpBitConverter.WriteBytes(buffer, segment, segment.Offset, segment.Count);
                }
            }
            else
            {
                IReadOnlyList<ArraySegment<byte>> listValue = (IReadOnlyList<ArraySegment<byte>>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    var segment = listValue[i];
                    AmqpBitConverter.WriteUInt(buffer, (uint)segment.Count);
                    AmqpBitConverter.WriteBytes(buffer, segment, segment.Offset, segment.Count);
                }
            }
        }

        public override ArraySegment<byte>[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            ArraySegment<byte>[] array = new ArraySegment<byte>[count];
            for (int i = 0; i < count; ++i)
            {
                AmqpEncoding.ReadCount(buffer, formatCode, FormatCode.Binary8, FormatCode.Binary32, out count);
                // always copy?
                byte[] valueBuffer = new byte[count];
                var arraySegment = new ArraySegment<byte>(valueBuffer, 0, count);
                AmqpBitConverter.ReadBytes(buffer, arraySegment, 0, count);
                array[i] = arraySegment;
            }
            return array;
        }
    }
}
