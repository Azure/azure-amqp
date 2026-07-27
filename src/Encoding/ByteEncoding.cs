// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class ByteEncoding : EncodingBase<sbyte>
    {
        public ByteEncoding()
            : base(FormatCode.Byte, FixedWidth.Byte)
        {
        }

        public static int GetEncodeSize(sbyte value)
        {
            return FixedWidth.ByteEncoded;
        }

        public static void Encode(sbyte value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Byte);
            AmqpBitConverter.WriteByte(buffer, value);
        }

        public static sbyte Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadByte(buffer);
        }

        public override int GetArrayValueSize(sbyte[] array)
        {
            return array.Length;
        }

        public override void WriteArrayValue(sbyte[] array, ByteBuffer buffer)
        {
            buffer.ValidateWrite(array.Length);
            Buffer.BlockCopy(array, 0, buffer.Buffer, buffer.WritePos, array.Length);
            buffer.Append(array.Length);
        }

        public override sbyte[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, sbyte[] array)
        {
            buffer.ValidateRead(array.Length);
            Buffer.BlockCopy(buffer.Buffer, buffer.Offset, array, 0, array.Length);
            buffer.Complete(array.Length);
            return array;
        }

        protected override int OnGetSize(sbyte value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.Byte : FixedWidth.ByteEncoded;
        }

        protected override void OnWrite(sbyte value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteByte(buffer, value);
            }
        }

        protected override sbyte OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
