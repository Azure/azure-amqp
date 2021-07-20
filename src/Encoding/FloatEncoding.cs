// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class FloatEncoding : EncodingBase<float>
    {
        public FloatEncoding()
            : base(FormatCode.Float, FixedWidth.Float)
        {
        }

        public static int GetEncodeSize(float value)
        {
            return FixedWidth.FloatEncoded;
        }

        public static void Encode(float value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Float);
            AmqpBitConverter.WriteFloat(buffer, value);
        }

        public static float Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadFloat(buffer);
        }

        public override int GetArrayValueSize(float[] array)
        {
            return array.Length * FixedWidth.Float;
        }

        public override void WriteArrayValue(float[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                AmqpBitConverter.WriteFloat(buffer, array[i]);
            }
        }

        public override float[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, float[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = AmqpBitConverter.ReadFloat(buffer);
            }

            return array;
        }

        protected override int OnGetSize(float value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.FloatEncoded : FixedWidth.Float;
        }

        protected override void OnWrite(float value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteFloat(buffer, value);
            }
        }

        protected override float OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadFloat(buffer);
        }
    }
}
