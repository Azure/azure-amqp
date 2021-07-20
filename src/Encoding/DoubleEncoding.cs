// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System.Runtime.CompilerServices;

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
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.Double)
            {
                AmqpBitConverter.WriteULong(buffer.Buffer, pos, Unsafe.As<double, ulong>(ref array[i]));
            }

            buffer.Append(size);
        }

        public override double[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, double[] array)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateRead(size);
            for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.Double)
            {
                ulong data = AmqpBitConverter.ReadULong(buffer.Buffer, pos, FixedWidth.ULong);
                array[i] = Unsafe.As<ulong, double>(ref data);
            }

            buffer.Complete(size);
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
