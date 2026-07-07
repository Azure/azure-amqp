// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

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

        internal static DescribedType Decode(ByteBuffer buffer, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            AmqpEncoding.VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Described);
            object descriptor = AmqpEncoding.DecodeObject(buffer, depth + 1, ref totalUnboundedSize);
            object value = AmqpEncoding.DecodeObject(buffer, depth + 1, ref totalUnboundedSize);
            return new DescribedType(descriptor, value);
        }

        public override DescribedType[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, DescribedType[] array)
        {
            int totalUnboundedSize = 0;
            return ReadArrayValue(buffer, formatCode, array, 0, ref totalUnboundedSize);
        }

        internal DescribedType[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, DescribedType[] array, int depth, ref int totalUnboundedSize)
        {
            object descriptor = AmqpEncoding.DecodeObject(buffer, depth + 1, ref totalUnboundedSize);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            EncodingBase encoding = AmqpEncoding.GetEncoding(formatCode);
            for (int i = 0; i < array.Length; i++)
            {
                object value = encoding.DecodeObject(buffer, formatCode, depth + 1, ref totalUnboundedSize);
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
            int totalUnboundedSize = 0;
            return Decode(buffer, formatCode, 0, ref totalUnboundedSize);
        }

        internal override object DecodeObject(ByteBuffer buffer, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            return Decode(buffer, formatCode, depth, ref totalUnboundedSize);
        }

        internal override Array DecodeArray(ByteBuffer buffer, FormatCode formatCode, int count, int depth, ref int totalUnboundedSize)
        {
            AmqpEncoding.TrackUnboundedSize(count, 0, buffer.Length, ref totalUnboundedSize);
            if (count == 0)
            {
                return Array.Empty<DescribedType>();
            }

            DescribedType[] array = new DescribedType[count];
            return ReadArrayValue(buffer, formatCode, array, depth, ref totalUnboundedSize);
        }

        static int GetArrayItemSize(DescribedType value, int index)
        {
            EncodingBase encoding = AmqpEncoding.GetEncoding(value.Value.GetType());
            int size = 0;
            if (index == 0)
            {
                size += AmqpEncoding.GetObjectEncodeSize(value.Descriptor);
                size += FixedWidth.FormatCode;
            }

            size += encoding.GetObjectEncodeSize(value.Value, true);
            return size;
        }

        static void EncodeArrayItem(DescribedType value, int index, ByteBuffer buffer)
        {
            EncodingBase encoding = AmqpEncoding.GetEncoding(value.Value.GetType());
            if (index == 0)
            {
                AmqpEncoding.EncodeObject(value.Descriptor, buffer);
                AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);
            }

            encoding.EncodeObject(value.Value, true, buffer);
        }
    }
}
