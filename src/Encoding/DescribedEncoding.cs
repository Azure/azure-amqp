// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    sealed class DescribedEncoding : EncodingBase<DescribedType>
    {
        public DescribedEncoding()
            : base(FormatCode.Described)
        {
        }

        public static int GetEncodeSize(DescribedType value)
        {
            int descriptorSize = AmqpEncoding.GetObjectEncodeSize(value.Descriptor);
            int valueSize = AmqpEncoding.GetObjectEncodeSize(value.Value);
            return FixedWidth.FormatCode + descriptorSize + valueSize;
        }

        public static void Encode(DescribedType value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Described);
            AmqpEncoding.EncodeObject(value.Descriptor, buffer);
            AmqpEncoding.EncodeObject(value.Value, buffer);
        }

        public static DescribedType Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            AmqpEncoding.VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Described);
            object descriptor = AmqpEncoding.DecodeObject(buffer);
            object value = AmqpEncoding.DecodeObject(buffer);
            return new DescribedType(descriptor, value);
        }

        public override DescribedType[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, DescribedType[] array)
        {
            object descriptor = AmqpEncoding.DecodeObject(buffer);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            IEncoding encoding = AmqpEncoding.GetEncoding(formatCode);
            for (int i = 0; i < array.Length; i++)
            {
                object value = encoding.Read(buffer, formatCode);
                array[i] = new DescribedType(descriptor, value);
            }

            return array;
        }

        protected override int OnGetSize(DescribedType value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : GetArrayItemSize(value, arrayIndex);
        }

        protected override void OnWrite(DescribedType value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                EncodeArrayItem(value, arrayIndex, buffer);
            }
        }

        protected override DescribedType OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }

        static int GetArrayItemSize(DescribedType value, int index)
        {
            int size = 0;
            if (index == 0)
            {
                size += AmqpEncoding.GetObjectEncodeSize(value.Descriptor);
                size += FixedWidth.FormatCode;
            }

            size += AmqpEncoding.GetObjectEncodeSize(value.Value) - FixedWidth.FormatCode;
            return size;
        }

        static void EncodeArrayItem(DescribedType value, int index, ByteBuffer buffer)
        {
            IEncoding encoding = AmqpEncoding.GetEncoding(value.Value.GetType());
            if (index == 0)
            {
                AmqpEncoding.EncodeObject(value.Descriptor, buffer);
                AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);
            }

            encoding.Write(value.Value, buffer, index);
        }
    }
}
