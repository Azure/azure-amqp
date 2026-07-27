// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Decoding from AMQP decimal to C# decimal can lose precision and
    /// can also cause OverflowException.
    /// </summary>
    sealed class DecimalEncoding : EncodingBase<decimal>
    {
        const int Decimal32Bias = 101;
        const int Decimal64Bias = 398;
        const int Decimal128Bias = 6176;
        static readonly bool decimalDataLayoutCompatible = IsDecimalDataLayoutCompatible();

        public DecimalEncoding()
            : base(FormatCode.Decimal128)
        {
        }

        public static int GetEncodeSize(decimal value)
        {
            return FixedWidth.Decimal128Encoded;
        }

        public static void Encode(decimal value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Decimal128);
            EncodeValue(value, buffer);
        }

        public static decimal Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return DecodeValue(buffer, formatCode);
        }

        public override int GetArrayValueSize(decimal[] array)
        {
            return array.Length * FixedWidth.Decimal128;
        }

        public override void WriteArrayValue(decimal[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                EncodeValue(array[i], buffer);
            }
        }

        public override decimal[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, decimal[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = DecodeValue(buffer, formatCode);
            }

            return array;
        }

        protected override int OnGetSize(decimal value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.Decimal128Encoded : FixedWidth.Decimal128;
        }

        protected override void OnWrite(decimal value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                EncodeValue(value, buffer);
            }
        }

        protected override decimal OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return DecodeValue(buffer, formatCode);
        }

        [StructLayout(LayoutKind.Explicit)]
        struct DecimalData
        {
            [FieldOffset(0)]
            public decimal Value;

            [FieldOffset(0)]
            public uint Flags;

            [FieldOffset(4)]
            public uint High;

            [FieldOffset(8)]
            public uint Low;

            [FieldOffset(12)]
            public uint Mid;
        }

        static bool IsDecimalDataLayoutCompatible()
        {
            decimal value = new decimal(
                unchecked((int)0x89ABCDEF),
                0x01234567,
                0x76543210,
                true,
                28);

            int[] bits = decimal.GetBits(value);
            var data = new DecimalData { Value = value };

            return data.Low == unchecked((uint)bits[0])
                && data.Mid == unchecked((uint)bits[1])
                && data.High == unchecked((uint)bits[2])
                && data.Flags == unchecked((uint)bits[3]);
        }

        static void GetDecimalBits(decimal value, Span<uint> destination)
        {
            if (destination.Length < 4)
            {
                throw new ArgumentException("Destination is too short.", nameof(destination));
            }

            if (decimalDataLayoutCompatible)
            {
                var data = new DecimalData { Value = value };
                destination[0] = data.Low;
                destination[1] = data.Mid;
                destination[2] = data.High;
                destination[3] = data.Flags;
                return;
            }

            // Portable fallback for big-endian or otherwise unusual runtimes.
            // It allocates, but correctness takes priority on this uncommon path.
            int[] bits = decimal.GetBits(value);
            destination[0] = (uint)bits[0];
            destination[1] = (uint)bits[1];
            destination[2] = (uint)bits[2];
            destination[3] = (uint)bits[3];
        }

        static void EncodeValue(decimal value, ByteBuffer buffer)
        {
            Span<uint> bits = stackalloc uint[4];
            GetDecimalBits(value, bits);

            uint low = bits[0];
            uint middle = bits[1];
            uint high = bits[2];
            uint flags = bits[3];

            int scale = (byte)(flags >> 16);
            int exponent = Decimal128Bias - scale;

            Span<byte> bytes = stackalloc byte[FixedWidth.Decimal128];

            // decimal128 finite-value layout:
            // sign + upper 7 exponent bits, lower 7 exponent bits, coefficient.
            bytes[0] = (byte)((flags >> 24) | (uint)(exponent >> 7));
            bytes[1] = (byte)((exponent & 0x7F) << 1);
            bytes[2] = 0;
            bytes[3] = 0;

            BinaryPrimitives.WriteUInt32BigEndian(bytes.Slice(4, 4), high);
            BinaryPrimitives.WriteUInt32BigEndian(bytes.Slice(8, 4), middle);
            BinaryPrimitives.WriteUInt32BigEndian(bytes.Slice(12, 4), low);

            AmqpBitConverter.WriteBytes(buffer, bytes, 0, bytes.Length);
        }

        static decimal DecodeValue(ByteBuffer buffer, FormatCode formatCode)
        {
            decimal value = 0;
            switch (formatCode)
            {
                case FormatCode.Decimal32:
                    value = DecimalEncoding.DecodeDecimal32(buffer);
                    break;
                case FormatCode.Decimal64:
                    value = DecimalEncoding.DecodeDecimal64(buffer);
                    break;
                case FormatCode.Decimal128:
                    value = DecimalEncoding.DecodeDecimal128(buffer);
                    break;
                default:
                    throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            return value;
        }

        static decimal DecodeDecimal32(ByteBuffer buffer)
        {
            Span<byte> bytes = stackalloc byte[FixedWidth.Decimal32];
            AmqpBitConverter.ReadBytes(buffer, bytes, 0, bytes.Length);
            int sign = 1;
            int exponent = 0;

            sign = (bytes[0] & 0x80) != 0 ? -1 : 1;
            if ((bytes[0] & 0x60) != 0x60)
            {
                // s 8-bit-exponent (0)23-bit-significant
                exponent = ((bytes[0] & 0x7F) << 1) | ((bytes[1] & 0x80) >> 7);
                bytes[0] = 0;
                bytes[1] &= 0x7F;
            }
            else if ((bytes[0] & 0x78) != 0)
            {
                // handle NaN and Infinity
            }
            else
            {
                // s 11 8-bit-exponent (100)21-bit-significant
                exponent = ((bytes[0] & 0x1F) << 3) | ((bytes[1] & 0xE0) >> 5);
                bytes[0] = 0;
                bytes[1] &= 0x1F;
                bytes[1] |= 0x80;
            }

            int low = (int)AmqpBitConverter.ReadUInt(bytes, 0, bytes.Length);
            return CreateDecimal(low, 0, 0, sign, exponent - Decimal32Bias);
        }

        static decimal DecodeDecimal64(ByteBuffer buffer)
        {
            Span<byte> bytes = stackalloc byte[FixedWidth.Decimal64];
            AmqpBitConverter.ReadBytes(buffer, bytes, 0, bytes.Length);
            int sign = 1;
            int exponent = 0;

            sign = (bytes[0] & 0x80) != 0 ? -1 : 1;
            if ((bytes[0] & 0x60) != 0x60)
            {
                // s 10-bit-exponent (0)53-bit-significant
                exponent = ((bytes[0] & 0x7F) << 3) | ((bytes[1] & 0xE0) >> 5);
                bytes[0] = 0;
                bytes[1] &= 0x1F;
            }
            else if ((bytes[0] & 0x78) != 0)
            {
                // handle NaN and Infinity
            }
            else
            {
                // s 11 10-bit-exponent (100)51-bit-significant
                exponent = ((bytes[0] & 0x1F) << 8) | ((bytes[1] & 0xF8) >> 3);
                bytes[0] = 0;
                bytes[1] &= 0x7;
                bytes[1] |= 0x20;
            }

            int middle = (int)AmqpBitConverter.ReadUInt(bytes, 0, 4);
            int low = (int)AmqpBitConverter.ReadUInt(bytes, 4, 4);
            return CreateDecimal(low, middle, 0, sign, exponent - Decimal64Bias);
        }

        static decimal DecodeDecimal128(ByteBuffer buffer)
        {
            Span<byte> bytes = stackalloc byte[FixedWidth.Decimal128];
            AmqpBitConverter.ReadBytes(buffer, bytes, 0, bytes.Length);
            int sign = 1;
            int exponent = 0;

            sign = (bytes[0] & 0x80) != 0 ? -1 : 1;
            if ((bytes[0] & 0x60) != 0x60)
            {
                // s 14-bit-exponent (0)113-bit-significant
                exponent = ((bytes[0] & 0x7F) << 7) | ((bytes[1] & 0xFE) >> 1);
                bytes[0] = 0;
                bytes[1] &= 0x1;
            }
            else if ((bytes[0] & 0x78) != 0)
            {
                // handle NaN and Infinity
            }
            else
            {
                // s 11 14-bit-exponent (100)111-bit-significant
                // it is out of the valid range already. Should not be used
                return 0;
            }

            int high = (int)AmqpBitConverter.ReadUInt(bytes, 4, 4);
            int middle = (int)AmqpBitConverter.ReadUInt(bytes, 8, 4);
            int low = (int)AmqpBitConverter.ReadUInt(bytes, 12, 4);
            return CreateDecimal(low, middle, high, sign, exponent - Decimal128Bias);
        }

        static decimal CreateDecimal(int low, int middle, int high, int sign, int exponent)
        {
            if (exponent <= 0)
            {
                return new decimal(low, middle, high, sign < 0, (byte)-exponent);
            }

            decimal value = new decimal(low, middle, high, sign < 0, 0);
            for (int i = 0; i < exponent; ++i)
            {
                value *= 10;
            }

            return value;
        }
    }
}
