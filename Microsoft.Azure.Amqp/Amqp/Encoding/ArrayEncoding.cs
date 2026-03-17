// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.CompilerServices;
    using System.Text;

    sealed class ArrayEncoding : EncodingBase
    {
        public ArrayEncoding()
            : base(FormatCode.Array32)
        {
        }

        public static int GetEncodeSize<T>(IReadOnlyList<T> value)
        {
            Debug.Assert(value != null);
            int valueSize = 0;
#if NET8_0_OR_GREATER
            foreach (T item in value)
            {
                if (valueSize == 0 && typeof(T) == typeof(DescribedType))
                {
                    valueSize += AmqpCodec.GetObjectEncodeSize(Unsafe.As<DescribedType>(item).Descriptor) + FixedWidth.FormatCode;
                }
                valueSize += GetValueSize(item);
            }
#else
            var itemEncoding = AmqpEncoding.GetEncoding(typeof(T));
            foreach (T item in value)
            {
                if (valueSize == 0 && typeof(T) == typeof(DescribedType))
                {
                    valueSize += AmqpCodec.GetObjectEncodeSize(((DescribedType)(object)item).Descriptor) + FixedWidth.FormatCode;
                }
                valueSize += itemEncoding.GetObjectEncodeSize(item, true);
            }
#endif

            return FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int + FixedWidth.FormatCode + valueSize;
        }

        public static void Encode<T>(IReadOnlyList<T> value, ByteBuffer buffer)
        {
            Debug.Assert(value != null);
            var itemEncoding = AmqpEncoding.GetEncoding(typeof(T));
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Array32);
            int offset = buffer.WritePos;
            AmqpBitConverter.WriteInt(buffer, 0);
            AmqpBitConverter.WriteInt(buffer, value.Count);
            AmqpBitConverter.WriteUByte(buffer, itemEncoding.FormatCode);
            if (itemEncoding.FormatCode == FormatCode.Described && value.Count > 0)
            {
                var described = (DescribedType)(object)value[0];
                AmqpEncoding.EncodeObject(described.Descriptor, buffer);
                AmqpBitConverter.WriteUByte(buffer, AmqpEncoding.GetEncoding(described.Value).FormatCode);
            }

            foreach (T item in value)
            {
#if NET8_0_OR_GREATER
                EncodeValue(buffer, item);
#else
                itemEncoding.EncodeObject(item, true, buffer);
#endif
            }
            AmqpBitConverter.WriteUInt(buffer.Buffer, offset, (uint)(buffer.WritePos - offset - FixedWidth.Int));
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
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }

            var array = (Array)value;
            var itemEncoding = AmqpEncoding.GetEncoding(value.GetType().GetElementType());
            int valueSize = 0;
            if (itemEncoding.FormatCode == FormatCode.Described && array.Length > 0)
            {
                DescribedType described = (DescribedType)array.GetValue(0);
                valueSize += AmqpCodec.GetObjectEncodeSize(described.Descriptor) + FixedWidth.FormatCode;
            }
            foreach (var item in array)
            {
                valueSize += itemEncoding.GetObjectEncodeSize(item, true);
            }

            return FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int + FixedWidth.FormatCode + valueSize;
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
                return;
            }

            var array = (Array)value;
            var itemEncoding = AmqpEncoding.GetEncoding(value.GetType().GetElementType());
            if (!arrayEncoding)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Array32);
            }
            int offset = buffer.WritePos;
            AmqpBitConverter.WriteInt(buffer, 0);
            AmqpBitConverter.WriteInt(buffer, array.Length);
            AmqpBitConverter.WriteUByte(buffer, itemEncoding.FormatCode);
            if (itemEncoding.FormatCode == FormatCode.Described && array.Length > 0)
            {
                DescribedType described = (DescribedType)array.GetValue(0);
                AmqpEncoding.EncodeObject(described.Descriptor, buffer);
                AmqpBitConverter.WriteUByte(buffer, AmqpEncoding.GetEncoding(described.Value).FormatCode);
            }
            foreach (var item in array)
            {
                itemEncoding.EncodeObject(item, true, buffer);
            }
            AmqpBitConverter.WriteUInt(buffer.Buffer, offset, (uint)(buffer.WritePos - offset - FixedWidth.Int));
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
                case FormatCode.Described:
                    array = ArrayEncoding.Decode<DescribedType>(buffer, size, count, formatCode);
                    break;
                default:
                    throw new NotSupportedException(CommonResources.GetString(CommonResources.NotSupportFrameCode, formatCode));
            }

            return array;
        }

#if NET8_0_OR_GREATER
        // Returns the raw value size for a single element (no format code overhead).
        static int GetValueSize<T>(T value)
        {
            if (typeof(T) == typeof(bool)) return FixedWidth.BooleanVar;
            else if (typeof(T) == typeof(byte)) return FixedWidth.UByte;
            else if (typeof(T) == typeof(sbyte)) return FixedWidth.Byte;
            else if (typeof(T) == typeof(ushort)) return FixedWidth.UShort;
            else if (typeof(T) == typeof(short)) return FixedWidth.Short;
            else if (typeof(T) == typeof(uint)) return FixedWidth.UInt;
            else if (typeof(T) == typeof(int)) return FixedWidth.Int;
            else if (typeof(T) == typeof(ulong)) return FixedWidth.ULong;
            else if (typeof(T) == typeof(long)) return FixedWidth.Long;
            else if (typeof(T) == typeof(float)) return FixedWidth.Float;
            else if (typeof(T) == typeof(double)) return FixedWidth.Double;
            else if (typeof(T) == typeof(char)) return FixedWidth.Char;
            else if (typeof(T) == typeof(DateTime)) return FixedWidth.TimeStamp;
            else if (typeof(T) == typeof(Guid)) return FixedWidth.Uuid;
            else if (typeof(T) == typeof(ArraySegment<byte>))
            {
                return FixedWidth.UInt + Unsafe.As<T, ArraySegment<byte>>(ref value).Count;
            }
            else if (typeof(T) == typeof(AmqpSymbol))
            {
                return FixedWidth.UInt + Encoding.ASCII.GetByteCount(Unsafe.As<T, AmqpSymbol>(ref value).Value);
            }
            else
            {
                return AmqpEncoding.GetEncoding(typeof(T)).GetObjectEncodeSize(value, true);
            }
        }

        // Optimized path: writes raw value bytes directly (no per-element format code).
        // Avoid boxing for value types.
        static void EncodeValue<T>(ByteBuffer buffer, T value)
        {
            if (typeof(T) == typeof(bool))
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)(Unsafe.As<T, bool>(ref value) ? 1 : 0));
            }
            else if (typeof(T) == typeof(byte))
            {
                AmqpBitConverter.WriteUByte(buffer, Unsafe.As<T, byte>(ref value));
            }
            else if (typeof(T) == typeof(sbyte))
            {
                AmqpBitConverter.WriteByte(buffer, Unsafe.As<T, sbyte>(ref value));
            }
            else if (typeof(T) == typeof(ushort))
            {
                AmqpBitConverter.WriteUShort(buffer, Unsafe.As<T, ushort>(ref value));
            }
            else if (typeof(T) == typeof(short))
            {
                AmqpBitConverter.WriteShort(buffer, Unsafe.As<T, short>(ref value));
            }
            else if (typeof(T) == typeof(uint))
            {
                AmqpBitConverter.WriteUInt(buffer, Unsafe.As<T, uint>(ref value));
            }
            else if (typeof(T) == typeof(int))
            {
                AmqpBitConverter.WriteInt(buffer, Unsafe.As<T, int>(ref value));
            }
            else if (typeof(T) == typeof(ulong))
            {
                AmqpBitConverter.WriteULong(buffer, Unsafe.As<T, ulong>(ref value));
            }
            else if (typeof(T) == typeof(long))
            {
                AmqpBitConverter.WriteLong(buffer, Unsafe.As<T, long>(ref value));
            }
            else if (typeof(T) == typeof(float))
            {
                AmqpBitConverter.WriteFloat(buffer, Unsafe.As<T, float>(ref value));
            }
            else if (typeof(T) == typeof(double))
            {
                AmqpBitConverter.WriteDouble(buffer, Unsafe.As<T, double>(ref value));
            }
            else if (typeof(T) == typeof(char))
            {
                char c = Unsafe.As<T, char>(ref value);
                AmqpBitConverter.WriteInt(buffer, char.ConvertToUtf32(new string(c, 1), 0));
            }
            else if (typeof(T) == typeof(DateTime))
            {
                AmqpBitConverter.WriteLong(buffer, TimeStampEncoding.GetMilliseconds(Unsafe.As<T, DateTime>(ref value)));
            }
            else if (typeof(T) == typeof(Guid))
            {
                AmqpBitConverter.WriteUuid(buffer, Unsafe.As<T, Guid>(ref value));
            }
            else if (typeof(T) == typeof(ArraySegment<byte>))
            {
                var binaryValue = Unsafe.As<T, ArraySegment<byte>>(ref value);
                AmqpBitConverter.WriteUInt(buffer, (uint)binaryValue.Count);
                AmqpBitConverter.WriteBytes(buffer, binaryValue.Array, binaryValue.Offset, binaryValue.Count);
            }
            else if (typeof(T) == typeof(AmqpSymbol))
            {
                string strValue = Unsafe.As<T, AmqpSymbol>(ref value).Value;
                AmqpEncoding.GetEncoding(typeof(string)).EncodeObject(strValue, true, buffer);
            }
            else
            {
                AmqpEncoding.GetEncoding(typeof(T)).EncodeObject(value, true, buffer);
            }
        }

        // Optimized path: reads raw value bytes directly, returning T without boxing.
        static T DecodeValue<T>(ByteBuffer buffer, FormatCode formatCode)
        {
            if (typeof(T) == typeof(bool))
            {
                bool result = formatCode == FormatCode.BooleanTrue
                    ? true
                    : formatCode == FormatCode.BooleanFalse
                    ? false
                    : AmqpBitConverter.ReadUByte(buffer) != 0;
                return Unsafe.As<bool, T>(ref result);
            }
            else if (typeof(T) == typeof(byte))
            {
                byte result = AmqpBitConverter.ReadUByte(buffer);
                return Unsafe.As<byte, T>(ref result);
            }
            else if (typeof(T) == typeof(sbyte))
            {
                sbyte result = AmqpBitConverter.ReadByte(buffer);
                return Unsafe.As<sbyte, T>(ref result);
            }
            else if (typeof(T) == typeof(ushort))
            {
                ushort result = AmqpBitConverter.ReadUShort(buffer);
                return Unsafe.As<ushort, T>(ref result);
            }
            else if (typeof(T) == typeof(short))
            {
                short result = AmqpBitConverter.ReadShort(buffer);
                return Unsafe.As<short, T>(ref result);
            }
            else if (typeof(T) == typeof(uint))
            {
                uint result = formatCode == FormatCode.UInt0
                    ? 0
                    : formatCode == FormatCode.SmallUInt
                    ? AmqpBitConverter.ReadUByte(buffer)
                    : AmqpBitConverter.ReadUInt(buffer);
                return Unsafe.As<uint, T>(ref result);
            }
            else if (typeof(T) == typeof(int))
            {
                int result = formatCode == FormatCode.SmallInt
                    ? AmqpBitConverter.ReadByte(buffer)
                    : AmqpBitConverter.ReadInt(buffer);
                return Unsafe.As<int, T>(ref result);
            }
            else if (typeof(T) == typeof(ulong))
            {
                ulong result = formatCode == FormatCode.ULong0
                    ? 0
                    : formatCode == FormatCode.SmallULong
                    ? AmqpBitConverter.ReadUByte(buffer)
                    : AmqpBitConverter.ReadULong(buffer);
                return Unsafe.As<ulong, T>(ref result);
            }
            else if (typeof(T) == typeof(long))
            {
                long result = formatCode == FormatCode.SmallLong
                    ? AmqpBitConverter.ReadByte(buffer)
                    : AmqpBitConverter.ReadLong(buffer);
                return Unsafe.As<long, T>(ref result);
            }
            else if (typeof(T) == typeof(float))
            {
                float result = AmqpBitConverter.ReadFloat(buffer);
                return Unsafe.As<float, T>(ref result);
            }
            else if (typeof(T) == typeof(double))
            {
                double result = AmqpBitConverter.ReadDouble(buffer);
                return Unsafe.As<double, T>(ref result);
            }
            else if (typeof(T) == typeof(char))
            {
                char result = char.ConvertFromUtf32(AmqpBitConverter.ReadInt(buffer))[0];
                return Unsafe.As<char, T>(ref result);
            }
            else if (typeof(T) == typeof(DateTime))
            {
                DateTime result = TimeStampEncoding.ToDateTime(AmqpBitConverter.ReadLong(buffer));
                return Unsafe.As<DateTime, T>(ref result);
            }
            else if (typeof(T) == typeof(Guid))
            {
                Guid result = AmqpBitConverter.ReadUuid(buffer);
                return Unsafe.As<Guid, T>(ref result);
            }
            else if (typeof(T) == typeof(AmqpSymbol))
            {
                AmqpSymbol result = SymbolEncoding.Decode(buffer, formatCode);
                return Unsafe.As<AmqpSymbol, T>(ref result);
            }
            else
            {
                // Reference types and variable-size types - fall back to DecodeObject
                return (T)AmqpEncoding.GetEncoding(formatCode).DecodeObject(buffer, formatCode);
            }
        }
#endif

        static T[] Decode<T>(ByteBuffer buffer, int size, int count, FormatCode formatCode)
        {
            T[] array = new T[count];
            EncodingBase encoding = AmqpEncoding.GetEncoding(formatCode);
            object descriptor = null;
            if (formatCode == FormatCode.Described)
            {
                descriptor = AmqpEncoding.DecodeObject(buffer);
                formatCode = AmqpEncoding.ReadFormatCode(buffer);
                encoding = AmqpEncoding.GetEncoding(formatCode);
            }

#if NET8_0_OR_GREATER
            for (int i = 0; i < count; ++i)
            {
                if (descriptor != null)
                {
                    object value = new DescribedType(descriptor, encoding.DecodeObject(buffer, formatCode));
                    array[i] = (T)value;
                }
                else
                {
                    array[i] = DecodeValue<T>(buffer, formatCode);
                }
            }
#else
            {
                for (int i = 0; i < count; ++i)
                {
                    object value = encoding.DecodeObject(buffer, formatCode);
                    if (descriptor != null)
                    {
                        value = new DescribedType(descriptor, value);
                    }

                    array[i] = (T)value;
                }
            }
#endif

            return array;
        }
    }
}
