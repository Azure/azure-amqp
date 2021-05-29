// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;

    sealed class FloatEncoding : PrimitiveEncoding<float>
    {
        public FloatEncoding()
            : base(FormatCode.Float)
        {
        }

        public static int GetEncodeSize(float? value)
        {
            return value.HasValue ? FixedWidth.FloatEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(float? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Float);
                AmqpBitConverter.WriteFloat(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static float? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadFloat(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.Float : FloatEncoding.GetEncodeSize((float)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteFloat(buffer, (float)value);
            }
            else
            {
                FloatEncoding.Encode((float)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return FloatEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<float> value)
        {
            return FixedWidth.Float * value.Count;
        }

        public override void EncodeArray(IList<float> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.Float * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is float[] floatArray)
            {
                // fast-path for float[] so the bounds checks can be elided
                for (int i = 0; i < floatArray.Length; i++)
                {
                    var source = floatArray[i];
                    int floatAsInt = Unsafe.As<float, int>(ref source);
                    BinaryPrimitives.WriteInt32BigEndian(destination.Slice(FixedWidth.Float * i), floatAsInt);
                }
            }
            else
            {
                IReadOnlyList<float> listValue = (IReadOnlyList<float>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    var source = listValue[i];
                    int floatAsInt = Unsafe.As<float, int>(ref source);
                    BinaryPrimitives.WriteInt32BigEndian(destination.Slice(FixedWidth.Float * i), floatAsInt);
                }
            }

            buffer.Append(byteCount);
        }

        public override float[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.Float * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            float[] array = new float[count];
            for (int i = 0; i < count; ++i)
            {
                var value = BinaryPrimitives.ReadInt32BigEndian(source.Slice(FixedWidth.Float * i));
                array[i] = Unsafe.As<int, float>(ref value);
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
