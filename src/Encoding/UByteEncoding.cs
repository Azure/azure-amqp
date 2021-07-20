// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class UByteEncoding : EncodingBase<byte>
    {
        public UByteEncoding()
            : base(FormatCode.UByte, FixedWidth.UByte)
        {
        }

        public static int GetEncodeSize(byte value)
        {
            return FixedWidth.UByteEncoded;
        }

        public static void Encode(byte value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.UByte);
            AmqpBitConverter.WriteUByte(buffer, value);
        }

        public static byte Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadUByte(buffer);
        }

        public override int GetArrayValueSize(byte[] array)
        {
            return array.Length;
        }

        public override void WriteArrayValue(byte[] array, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteBytes(buffer, array, 0, array.Length);
        }

        public override byte[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, byte[] array)
        {
            buffer.ValidateRead(array.Length);
            Buffer.BlockCopy(buffer.Buffer, buffer.Offset, array, 0, array.Length);
            buffer.Complete(array.Length);
            return array;
        }

        protected override int OnGetSize(byte value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.UByteEncoded : FixedWidth.UByte;
        }

        protected override void OnWrite(byte value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteUByte(buffer, value);
            }
        }

        protected override byte OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
