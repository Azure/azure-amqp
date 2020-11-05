// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using Microsoft.Azure.Amqp.Framing;
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;

    sealed class ArrayEncoding : EncodingBase
    {
        public ArrayEncoding()
            : base(FormatCode.Array32)
        {
        }

        public static int GetEncodeSize<T>(IList value)
        {
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }
            else
            {
                Debug.Assert(value is T[] || value is Multiple<T>);

                return ArrayEncoding.GetEncodeSize(value, typeof(T), false);
            }
        }

        public static void Encode<T>(IList value, ByteBuffer buffer)
        {
            if (value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                Debug.Assert(value is T[] || value is Multiple<T>);

                int width;
                int encodeSize = ArrayEncoding.GetEncodeSize(value, typeof(T), false, out width);
                AmqpBitConverter.WriteUByte(buffer, width == FixedWidth.UByte ? FormatCode.Array8 : FormatCode.Array32);
                ArrayEncoding.Encode(value, typeof(T), width, encodeSize, buffer);
            }
        }

        public static T[] Decode<T>(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            int size;
            int count;
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out size, out count);

            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            return ArrayEncoding.Decode<T>(buffer, size, count, formatCode);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            Array array = (Array)value;
            Type elementType = array.GetType().GetElementType();

            return ArrayEncoding.GetEncodeSize(array, elementType, arrayEncoding);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            Array array = (Array)value;
            Type elementType = array.GetType().GetElementType();

            int width;
            int encodeSize = ArrayEncoding.GetEncodeSize(array, elementType, arrayEncoding, out width);
            AmqpBitConverter.WriteUByte(buffer, width == FixedWidth.UByte ? FormatCode.Array8 : FormatCode.Array32);
            ArrayEncoding.Encode(array, elementType, width, encodeSize, buffer);
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            int size = 0;
            int count = 0;
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out size, out count);

            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            Array array = null;
            switch (formatCode)
            {
                case FormatCode.Boolean:
                    array = ArrayEncoding.Decode<bool>(buffer, size, count, formatCode);
                    break;
                case FormatCode.UByte:
                    array = ArrayEncoding.Decode<byte>(buffer, size, count, formatCode);
                    break;
                case FormatCode.UShort:
                    array = ArrayEncoding.Decode<ushort>(buffer, size, count, formatCode);
                    break;
                case FormatCode.UInt:
                case FormatCode.SmallUInt:
                    array = ArrayEncoding.Decode<uint>(buffer, size, count, formatCode);
                    break;
                case FormatCode.ULong:
                case FormatCode.SmallULong:
                    array = ArrayEncoding.Decode<ulong>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Byte:
                    array = ArrayEncoding.Decode<sbyte>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Short:
                    array = ArrayEncoding.Decode<short>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Int:
                case FormatCode.SmallInt:
                    array = ArrayEncoding.Decode<int>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Long:
                case FormatCode.SmallLong:
                    array = ArrayEncoding.Decode<long>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Float:
                    array = ArrayEncoding.Decode<float>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Double:
                    array = ArrayEncoding.Decode<double>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Char:
                    array = ArrayEncoding.Decode<char>(buffer, size, count, formatCode);
                    break;
                case FormatCode.TimeStamp:
                    array = ArrayEncoding.Decode<DateTime>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Uuid:
                    array = ArrayEncoding.Decode<Guid>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Binary32:
                case FormatCode.Binary8:
                    array = ArrayEncoding.Decode<ArraySegment<byte>>(buffer, size, count, formatCode);
                    break;
                case FormatCode.String32Utf8:
                case FormatCode.String8Utf8:
                    array = ArrayEncoding.Decode<string>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Symbol32:
                case FormatCode.Symbol8:
                    array = ArrayEncoding.Decode<AmqpSymbol>(buffer, size, count, formatCode);
                    break;
                case FormatCode.List32:
                case FormatCode.List8:
                    array = ArrayEncoding.Decode<IList>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Map32:
                case FormatCode.Map8:
                    array = ArrayEncoding.Decode<AmqpMap>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Array32:
                case FormatCode.Array8:
                    array = ArrayEncoding.Decode<Array>(buffer, size, count, formatCode);
                    break;
                default:
                    throw new NotSupportedException(CommonResources.GetString(CommonResources.NotSupportFrameCode, formatCode));
            }

            return array;
        }

        static int GetEncodeSize(IList array, Type elementType, bool arrayEncoding)
        {
            int unused;
            return ArrayEncoding.GetEncodeSize(array, elementType, arrayEncoding, out unused);
        }

        static int GetEncodeSize(IList array, Type elementType, bool arrayEncoding, out int width)
        {
            int size = FixedWidth.FormatCode + ArrayEncoding.GetValueSize(array, elementType);
            width = arrayEncoding ? FixedWidth.UInt : AmqpEncoding.GetEncodeWidthByCountAndSize(array.Count, size);
            size += FixedWidth.FormatCode + width + width;
            return size;
        }

        static int GetValueSize(IList value, Type elementType)
        {
            if (value.Count == 0)
            {
                return 0;
            }

            EncodingBase encoding = AmqpEncoding.GetEncoding(elementType);
            if (encoding is PrimitiveEncoding primitiveEncoding)
            {
                return primitiveEncoding.GetArrayEncodeSize(value);
            }

            int valueSize = 0;
            foreach (object item in value)
            {
                bool arrayEncoding = true;
                if (encoding.FormatCode == FormatCode.Described && valueSize == 0)
                {
                    arrayEncoding = false;
                }

                valueSize += encoding.GetObjectEncodeSize(item, arrayEncoding);
            }

            return valueSize;
        }

        static void Encode(IList value, Type elementType, int width, int encodeSize, ByteBuffer buffer)
        {
            encodeSize -= FixedWidth.FormatCode + width;
            if (width == FixedWidth.UByte)
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)encodeSize);
                AmqpBitConverter.WriteUByte(buffer, (byte)value.Count);
            }
            else
            {
                AmqpBitConverter.WriteUInt(buffer, (uint)encodeSize);
                AmqpBitConverter.WriteUInt(buffer, (uint)value.Count);
            }

            if (value.Count > 0)
            {
                EncodingBase encoding = AmqpEncoding.GetEncoding(elementType);
                AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);

                if (encoding is PrimitiveEncoding primitiveEncoding)
                {
                    primitiveEncoding.EncodeArray(value, buffer);
                }
                else
                {
                    if (encoding.FormatCode == FormatCode.Described)
                    {
                        object firstItem = value[0];
                        DescribedType describedValue = (DescribedType)firstItem;
                        AmqpEncoding.EncodeObject(describedValue.Descriptor, buffer);
                        AmqpBitConverter.WriteUByte(buffer, AmqpEncoding.GetEncoding(describedValue.Value).FormatCode);
                    }

                    foreach (object item in value)
                    {
                        encoding.EncodeObject(item, true, buffer);
                    }
                }
            }
        }

        static T[] Decode<T>(ByteBuffer buffer, int size, int count, FormatCode formatCode)
        {
            EncodingBase encoding = AmqpEncoding.GetEncoding(formatCode);
            if (encoding is PrimitiveEncoding primitiveEncoding)
            {
                return (T[])primitiveEncoding.DecodeArray(buffer, count, formatCode);
            }

            object descriptor = null;
            if (formatCode == FormatCode.Described)
            {
                descriptor = AmqpEncoding.DecodeObject(buffer);
                formatCode = AmqpEncoding.ReadFormatCode(buffer);
            }

            T[] array = new T[count];
            for (int i = 0; i < count; ++i)
            {
                object value = encoding.DecodeObject(buffer, formatCode);
                if (descriptor != null)
                {
                    value = new DescribedType(descriptor, value);
                }

                array[i] = (T)value;
            }

            return array;
        }
    }
}
