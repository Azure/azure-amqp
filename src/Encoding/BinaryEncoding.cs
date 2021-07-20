// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Diagnostics;

    sealed class BinaryEncoding : EncodingBase<ArraySegment<byte>>
    {
        public BinaryEncoding()
            : base(FormatCode.Binary32)
        {
        }

        public static int GetEncodeSize(ArraySegment<byte> value)
        {
            if (value.Array == null)
            {
                return FixedWidth.NullEncoded;
            }

            return FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(value.Count) + value.Count;
        }

        public static void Encode(ArraySegment<byte> value, ByteBuffer buffer)
        {
            if (value.Array == null)
            {
                AmqpEncoding.EncodeNull(buffer);
                return;
            }

            if (value.Count <= byte.MaxValue)
            {
                AmqpBitConverter.Write(buffer, FormatCode.Binary8, (byte)value.Count);
            }
            else
            {
                AmqpBitConverter.Write(buffer, FormatCode.Binary32, (uint)value.Count);
            }

            AmqpBitConverter.WriteBytes(buffer, value.Array, value.Offset, value.Count);
        }

        public static ArraySegment<byte> Decode(ByteBuffer buffer, FormatCode formatCode, bool copy)
        {
            Debug.Assert(formatCode > 0);
            AmqpEncoding.ReadCount(buffer, formatCode, FormatCode.Binary8, FormatCode.Binary32, out int count);
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

        protected override int OnGetSize(ArraySegment<byte> value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Int + value.Count;
        }

        protected override void OnWrite(ArraySegment<byte> value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteInt(buffer, value.Count);
                AmqpBitConverter.WriteBytes(buffer, value.Array, value.Offset, value.Count);
            }
        }

        protected override ArraySegment<byte> OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode, true);
        }
    }
}
