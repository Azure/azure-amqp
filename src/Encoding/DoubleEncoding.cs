// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class DoubleEncoding : EncodingBase<double>
    {
        public DoubleEncoding()
            : base(FormatCode.Double, FixedWidth.Double)
        {
        }

        public static int GetEncodeSize(double value)
        {
            return FixedWidth.DoubleEncoded;
        }

        public static void Encode(double value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Double);
            AmqpBitConverter.WriteDouble(buffer, value);
        }

        public static double Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadDouble(buffer);
        }

        public override int GetArrayValueSize(double[] array)
        {
            return array.Length * FixedWidth.Double;
        }

        public override void WriteArrayValue(double[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                AmqpBitConverter.WriteDouble(buffer, array[i]);
            }
        }

        public override double[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, double[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = AmqpBitConverter.ReadDouble(buffer);
            }

            return array;
        }

        protected override int OnGetSize(double value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.DoubleEncoded : FixedWidth.Double;
        }

        protected override void OnWrite(double value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteDouble(buffer, value);
            }
        }

        protected override double OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadDouble(buffer);
        }
    }
}
