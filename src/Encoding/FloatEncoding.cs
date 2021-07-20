// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System.Runtime.CompilerServices;

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
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.Float)
            {
                AmqpBitConverter.WriteUInt(buffer.Buffer, pos, Unsafe.As<float, uint>(ref array[i]));
            }

            buffer.Append(size);
        }

        public override float[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, float[] array)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateRead(size);
            for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.Float)
            {
                uint data = AmqpBitConverter.ReadUInt(buffer.Buffer, pos, FixedWidth.UInt);
                array[i] = Unsafe.As<uint, float>(ref data);
            }

            buffer.Complete(size);

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
