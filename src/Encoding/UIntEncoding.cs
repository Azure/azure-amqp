// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class UIntEncoding : EncodingBase<uint>
    {
        public UIntEncoding()
            : base(FormatCode.UInt)
        {
        }

        public static int GetEncodeSize(uint value)
        {
            return value == 0u ? FixedWidth.FormatCode : (value <= byte.MaxValue ? FixedWidth.UByteEncoded : FixedWidth.UIntEncoded);
        }

        public static void Encode(uint value, ByteBuffer buffer)
        {
            if (value == 0u)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.UInt0);
            }
            else if (value <= byte.MaxValue)
            {
                AmqpBitConverter.Write(buffer, FormatCode.SmallUInt, (byte)value);
            }
            else
            {
                AmqpBitConverter.Write(buffer, FormatCode.UInt, value);
            }
        }

        public static uint Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == FormatCode.UInt0)
            {
                return 0u;
            }
            else if (formatCode == FormatCode.SmallUInt)
            {
                return AmqpBitConverter.ReadUByte(buffer);
            }
            else if (formatCode == FormatCode.UInt)
            {
                return AmqpBitConverter.ReadUInt(buffer);
            }

            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
        }

        public override int GetArrayValueSize(uint[] array)
        {
            return FixedWidth.UInt * array.Length;
        }

        public override void WriteArrayValue(uint[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                AmqpBitConverter.WriteUInt(buffer, array[i]);
            }
        }

        public override uint[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, uint[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = Decode(buffer, formatCode);
            }

            return array;
        }

        protected override int OnGetSize(uint value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.UInt;
        }

        protected override void OnWrite(uint value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteUInt(buffer, value);
            }
        }

        protected override uint OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
