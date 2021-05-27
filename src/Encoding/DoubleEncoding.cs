// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;

    sealed class DoubleEncoding : PrimitiveEncoding<double>
    {
        public DoubleEncoding()
            : base(FormatCode.Double)
        {
        }

        public static int GetEncodeSize(double? value)
        {
            return value.HasValue ? FixedWidth.DoubleEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(double? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Double);
                AmqpBitConverter.WriteDouble(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static double? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadDouble(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.Double : DoubleEncoding.GetEncodeSize((double)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteDouble(buffer, (double)value);
            }
            else
            {
                DoubleEncoding.Encode((double)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return DoubleEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<double> value)
        {
            return FixedWidth.Double * value.Count;
        }

        public override void EncodeArray(IList<double> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Double * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is double[] doubleArray)
            {
                // fast-path for double[] so the bounds checks can be elided
                for (int i = 0; i < doubleArray.Length; i++)
                {
                    long doubleAsLong = Unsafe.As<double, long>(ref doubleArray[i]);
                    BinaryPrimitives.WriteInt64BigEndian(destination.Slice(FixedWidth.Double * i), doubleAsLong);
                }
            }
            else
            {
                IReadOnlyList<double> listValue = (IReadOnlyList<double>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    var source = listValue[i];
                    long doubleAsLong = Unsafe.As<double, long>(ref source);
                    BinaryPrimitives.WriteInt64BigEndian(destination.Slice(FixedWidth.Double * i), doubleAsLong);
                }
            }

            buffer.Append(byteCount);
        }

        public override double[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.Double * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            double[] array = new double[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = BitConverter.Int64BitsToDouble(BinaryPrimitives.ReadInt64BigEndian(source.Slice(FixedWidth.Double * i)));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
