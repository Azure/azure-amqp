// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using Microsoft.Azure.Amqp.Framing;

    sealed class ArrayEncoding : EncodingBase
    {
        public ArrayEncoding()
            : base(FormatCode.Array32)
        {
        }

        public static int GetEncodeSize<T>(IList<T> value)
        {
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }

            Debug.Assert(value is T[] || value is Multiple<T>);

            return GetEncodeSize(value, false, out _);
        }

        public static void Encode<T>(IList<T> value, ByteBuffer buffer)
        {
            if (value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                Debug.Assert(value is T[] || value is Multiple<T>);

                int width;
                int encodeSize = GetEncodeSize(value, false, out width);
                AmqpBitConverter.WriteUByte(buffer, width == FixedWidth.UByte ? FormatCode.Array8 : FormatCode.Array32);
                Encode(value, width, encodeSize, buffer);
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
            return Decode<T>(buffer, size, count, formatCode);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            Array array = (Array)value;
            return GetEncodeSize(array, arrayEncoding, out _);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            Array array = (Array)value;
            int width;
            int encodeSize = GetEncodeSize(array, arrayEncoding, out width);
            AmqpBitConverter.WriteUByte(buffer, width == FixedWidth.UByte ? FormatCode.Array8 : FormatCode.Array32);
            ArrayEncoding.Encode(array, width, encodeSize, buffer);
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
                    array = Decode<bool>(buffer, size, count, formatCode);
                    break;
                case FormatCode.UByte:
                    array = Decode<byte>(buffer, size, count, formatCode);
                    break;
                case FormatCode.UShort:
                    array = Decode<ushort>(buffer, size, count, formatCode);
                    break;
                case FormatCode.UInt:
                case FormatCode.SmallUInt:
                    array = Decode<uint>(buffer, size, count, formatCode);
                    break;
                case FormatCode.ULong:
                case FormatCode.SmallULong:
                    array = Decode<ulong>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Byte:
                    array = Decode<sbyte>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Short:
                    array = Decode<short>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Int:
                case FormatCode.SmallInt:
                    array = Decode<int>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Long:
                case FormatCode.SmallLong:
                    array = Decode<long>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Float:
                    array = Decode<float>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Double:
                    array = Decode<double>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Char:
                    array = Decode<char>(buffer, size, count, formatCode);
                    break;
                case FormatCode.TimeStamp:
                    array = Decode<DateTime>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Uuid:
                    array = Decode<Guid>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Binary32:
                case FormatCode.Binary8:
                    array = Decode<ArraySegment<byte>>(buffer, size, count, formatCode);
                    break;
                case FormatCode.String32Utf8:
                case FormatCode.String8Utf8:
                    array = Decode<string>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Symbol32:
                case FormatCode.Symbol8:
                    array = Decode<AmqpSymbol>(buffer, size, count, formatCode);
                    break;
                case FormatCode.List32:
                case FormatCode.List8:
                    array = Decode<IList>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Map32:
                case FormatCode.Map8:
                    array = Decode<AmqpMap>(buffer, size, count, formatCode);
                    break;
                case FormatCode.Array32:
                case FormatCode.Array8:
                    array = Decode<Array>(buffer, size, count, formatCode);
                    break;
                default:
                    throw new NotSupportedException(CommonResources.GetString(CommonResources.NotSupportFrameCode, formatCode));
            }

            return array;
        }

        static int GetEncodeSize(Array array, bool arrayEncoding, out int width)
        {
            switch (array)
            {
                case bool[] concreteArray:
                    return GetEncodeSize<bool>(concreteArray, arrayEncoding, out width);
                case byte[] concreteArray:
                    return GetEncodeSize<byte>(concreteArray, arrayEncoding, out width);
                case ushort[] concreteArray:
                    return GetEncodeSize<ushort>(concreteArray, arrayEncoding, out width);
                case uint[] concreteArray:
                    return GetEncodeSize<uint>(concreteArray, arrayEncoding, out width);
                case ulong[] concreteArray:
                    return GetEncodeSize<ulong>(concreteArray, arrayEncoding, out width);
                case sbyte[] concreteArray:
                    return GetEncodeSize<sbyte>(concreteArray, arrayEncoding, out width);
                case short[] concreteArray:
                    return GetEncodeSize<short>(concreteArray, arrayEncoding, out width);
                case int[] concreteArray:
                    return GetEncodeSize<int>(concreteArray, arrayEncoding, out width);
                case long[] concreteArray:
                    return GetEncodeSize<long>(concreteArray, arrayEncoding, out width);
                case float[] concreteArray:
                    return GetEncodeSize<float>(concreteArray, arrayEncoding, out width);
                case double[] concreteArray:
                    return GetEncodeSize<double>(concreteArray, arrayEncoding, out width);
                case char[] concreteArray:
                    return GetEncodeSize<char>(concreteArray, arrayEncoding, out width);
                case DateTime[] concreteArray:
                    return GetEncodeSize<DateTime>(concreteArray, arrayEncoding, out width);
                case Guid[] concreteArray:
                    return GetEncodeSize<Guid>(concreteArray, arrayEncoding, out width);
                case ArraySegment<byte>[] concreteArray:
                    return GetEncodeSize<ArraySegment<byte>>(concreteArray, arrayEncoding, out width);
                case string[] concreteArray:
                    return GetEncodeSize<string>(concreteArray, arrayEncoding, out width);
                case AmqpSymbol[] concreteArray:
                    return GetEncodeSize<AmqpSymbol>(concreteArray, arrayEncoding, out width);
                default:
                    // TODO: better exception message
                    throw new NotSupportedException();
            }
                // case FormatCode.List32:
                // case FormatCode.List8:
                //     array = ArrayEncoding.Decode<IList>(buffer, size, count, formatCode);
                //     break;
                // case FormatCode.Map32:
                // case FormatCode.Map8:
                //     array = ArrayEncoding.Decode<AmqpMap>(buffer, size, count, formatCode);
                //     break;
                // case FormatCode.Array32:
                // case FormatCode.Array8:
                //     array = ArrayEncoding.Decode<Array>(buffer, size, count, formatCode);
        }

        static int GetEncodeSize<T>(IList<T> array, bool arrayEncoding, out int width)
        {
            int size = FixedWidth.FormatCode + GetValueSize(array);
            width = arrayEncoding ? FixedWidth.UInt : AmqpEncoding.GetEncodeWidthByCountAndSize(array.Count, size);
            size += FixedWidth.FormatCode + width + width;
            return size;
        }

        static int GetValueSize<T>(IList<T> value)
        {
            if (value.Count == 0)
            {
                return 0;
            }

            EncodingBase encoding = AmqpEncoding.GetEncoding(typeof(T));
            if (encoding is PrimitiveEncoding<T> primitiveEncoding)
            {
                return primitiveEncoding.GetArrayEncodeSize(value);
            }

            int valueSize = 0;
            for (var index = 0; index < value.Count; index++)
            {
                T item = value[index];
                bool arrayEncoding = !(encoding.FormatCode == FormatCode.Described && valueSize == 0);

                valueSize += encoding.GetObjectEncodeSize(item, arrayEncoding);
            }

            return valueSize;
        }

        static void Encode(Array array, int width, int encodeSize, ByteBuffer buffer)
        {
            switch (array)
            {
                case bool[] concreteArray:
                    Encode<bool>(concreteArray, width, encodeSize, buffer);
                    break;
                case byte[] concreteArray:
                    Encode<byte>(concreteArray, width, encodeSize, buffer);
                    break;
                case ushort[] concreteArray:
                    Encode<ushort>(concreteArray, width, encodeSize, buffer);
                    break;
                case uint[] concreteArray:
                    Encode<uint>(concreteArray, width, encodeSize, buffer);
                    break;
                case ulong[] concreteArray:
                    Encode<ulong>(concreteArray, width, encodeSize, buffer);
                    break;
                case sbyte[] concreteArray:
                    Encode<sbyte>(concreteArray, width, encodeSize, buffer);
                    break;
                case short[] concreteArray:
                    Encode<short>(concreteArray, width, encodeSize, buffer);
                    break;
                case int[] concreteArray:
                    Encode<int>(concreteArray, width, encodeSize, buffer);
                    break;
                case long[] concreteArray:
                    Encode<long>(concreteArray, width, encodeSize, buffer);
                    break;
                case float[] concreteArray:
                    Encode<float>(concreteArray, width, encodeSize, buffer);
                    break;
                case double[] concreteArray:
                    Encode<double>(concreteArray, width, encodeSize, buffer);
                    break;
                case char[] concreteArray:
                    Encode<char>(concreteArray, width, encodeSize, buffer);
                    break;
                case DateTime[] concreteArray:
                    Encode<DateTime>(concreteArray, width, encodeSize, buffer);
                    break;
                case Guid[] concreteArray:
                    Encode<Guid>(concreteArray, width, encodeSize, buffer);
                    break;
                case ArraySegment<byte>[] concreteArray:
                    Encode<ArraySegment<byte>>(concreteArray, width, encodeSize, buffer);
                    break;
                case string[] concreteArray:
                    Encode<string>(concreteArray, width, encodeSize, buffer);
                    break;
                case AmqpSymbol[] concreteArray:
                    Encode<AmqpSymbol>(concreteArray, width, encodeSize, buffer);
                    break;
                default:
                    // TODO: better exception message
                    throw new NotSupportedException();
            }

                // case FormatCode.List32:
                // case FormatCode.List8:
                //     array = ArrayEncoding.Decode<IList>(buffer, size, count, formatCode);
                //     break;
                // case FormatCode.Map32:
                // case FormatCode.Map8:
                //     array = ArrayEncoding.Decode<AmqpMap>(buffer, size, count, formatCode);
                //     break;
                // case FormatCode.Array32:
                // case FormatCode.Array8:
                //     array = ArrayEncoding.Decode<Array>(buffer, size, count, formatCode);
        }

        static void Encode<T>(IList<T> value, int width, int encodeSize, ByteBuffer buffer)
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
                EncodingBase encoding = AmqpEncoding.GetEncoding(typeof(T));
                AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);

                if (encoding is PrimitiveEncoding<T> primitiveEncoding)
                {
                    primitiveEncoding.EncodeArray(value, buffer);
                }
                else
                {
                    if (encoding.FormatCode == FormatCode.Described)
                    {
                        T firstItem = value[0];
                        // TODO: CHECK THIS
                        DescribedType describedValue = firstItem as DescribedType;
                        AmqpEncoding.EncodeObject(describedValue.Descriptor, buffer);
                        AmqpBitConverter.WriteUByte(buffer, AmqpEncoding.GetEncoding(describedValue.Value).FormatCode);
                    }

                    foreach (T item in value)
                    {
                        encoding.EncodeObject(item, true, buffer);
                    }
                }
            }
        }

        static T[] Decode<T>(ByteBuffer buffer, int size, int count, FormatCode formatCode)
        {
            EncodingBase encoding = AmqpEncoding.GetEncoding(formatCode);
            if (encoding is PrimitiveEncoding<T> primitiveEncoding)
            {
                return primitiveEncoding.DecodeArray(buffer, count, formatCode);
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
