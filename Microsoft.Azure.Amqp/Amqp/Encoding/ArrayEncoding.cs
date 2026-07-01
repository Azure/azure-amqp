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
        /// <summary>
        /// Initial allocation size and growth increment for dynamically-resized arrays.
        /// Ensures decoding correctness by allocating only what is needed as items are
        /// actually decoded, rather than trusting the count field in the payload.
        /// The actual max size of the array is bounded by the buffer length plus the max
        /// unbounded size for zero-width types.
        /// </summary>
        const int MaxInitialSize = 1024;
        const int MaxGrowFactor = 8 * 1024;

        public ArrayEncoding()
            : base(FormatCode.Array32)
        {
        }

        public static int GetEncodeSize<T>(IReadOnlyList<T> value)
        {
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }

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
            if (value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
                return;
            }

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

            int totalUnboundedSize = 0;
            return ArrayEncoding.Decode<T>(buffer, size, count, formatCode, 0, ref totalUnboundedSize);
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
            int totalUnboundedSize = 0;
            return DecodeObject(buffer, formatCode, 0, ref totalUnboundedSize);
        }

        internal override object DecodeObject(ByteBuffer buffer, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            AmqpEncoding.CheckMaxNestingDepth(depth);

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
                case FormatCode.BooleanTrue:
                case FormatCode.BooleanFalse:
                    array = ArrayEncoding.Decode<bool>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.UByte:
                    array = ArrayEncoding.Decode<byte>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.UShort:
                    array = ArrayEncoding.Decode<ushort>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.UInt:
                case FormatCode.SmallUInt:
                case FormatCode.UInt0:
                    array = ArrayEncoding.Decode<uint>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.ULong:
                case FormatCode.SmallULong:
                case FormatCode.ULong0:
                    array = ArrayEncoding.Decode<ulong>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Byte:
                    array = ArrayEncoding.Decode<sbyte>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Short:
                    array = ArrayEncoding.Decode<short>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Int:
                case FormatCode.SmallInt:
                    array = ArrayEncoding.Decode<int>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Long:
                case FormatCode.SmallLong:
                    array = ArrayEncoding.Decode<long>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Float:
                    array = ArrayEncoding.Decode<float>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Double:
                    array = ArrayEncoding.Decode<double>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Char:
                    array = ArrayEncoding.Decode<char>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.TimeStamp:
                    array = ArrayEncoding.Decode<DateTime>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Uuid:
                    array = ArrayEncoding.Decode<Guid>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Binary32:
                case FormatCode.Binary8:
                    array = ArrayEncoding.Decode<ArraySegment<byte>>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.String32Utf8:
                case FormatCode.String8Utf8:
                    array = ArrayEncoding.Decode<string>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Symbol32:
                case FormatCode.Symbol8:
                    array = ArrayEncoding.Decode<AmqpSymbol>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.List32:
                case FormatCode.List8:
                case FormatCode.List0:
                    array = ArrayEncoding.Decode<IList>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Map32:
                case FormatCode.Map8:
                    array = ArrayEncoding.Decode<AmqpMap>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Array32:
                case FormatCode.Array8:
                    array = ArrayEncoding.Decode<Array>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Described:
                    array = ArrayEncoding.Decode<DescribedType>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
                    break;
                case FormatCode.Decimal32:
                case FormatCode.Decimal64:
                case FormatCode.Decimal128:
                    array = ArrayEncoding.Decode<decimal>(buffer, size, count, formatCode, depth, ref totalUnboundedSize);
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
            else if (typeof(T) == typeof(decimal)) return FixedWidth.Decimal128;
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
            else if (typeof(T) == typeof(decimal))
            {
                DecimalEncoding.EncodeValue(Unsafe.As<T, decimal>(ref value), buffer);
            }
            else if (typeof(T) == typeof(char))
            {
                CharEncoding.EncodeValue(Unsafe.As<T, char>(ref value), buffer);
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
                SymbolEncoding.EncodeValue(Unsafe.As<T, AmqpSymbol>(ref value), buffer);
            }
            else
            {
                AmqpEncoding.GetEncoding(typeof(T)).EncodeObject(value, true, buffer);
            }
        }

        static Func<ByteBuffer, T> GetReader<T>(FormatCode formatCode, out int unboundedSize)
        {
            unboundedSize = 0;
            Func<ByteBuffer, T> reader = null;
            switch (formatCode)
            {
                case FormatCode.Boolean:
                    reader = static b => { bool r = AmqpBitConverter.ReadUByte(b) != 0; return Unsafe.As<bool, T>(ref r); };
                    break;
                case FormatCode.BooleanTrue:
                    unboundedSize = FixedWidth.BooleanVar;
                    reader = static b => { bool r = true; return Unsafe.As<bool, T>(ref r); };
                    break;
                case FormatCode.BooleanFalse:
                    unboundedSize = FixedWidth.BooleanVar;
                    reader = static b => { bool r = false; return Unsafe.As<bool, T>(ref r); };
                    break;
                case FormatCode.UByte:
                    reader = static b => { byte r = AmqpBitConverter.ReadUByte(b); return Unsafe.As<byte, T>(ref r); };
                    break;
                case FormatCode.UShort:
                    reader = static b => { ushort r = AmqpBitConverter.ReadUShort(b); return Unsafe.As<ushort, T>(ref r); };
                    break;
                case FormatCode.UInt0:
                    unboundedSize = FixedWidth.UInt;
                    reader = static b => { uint r = 0; return Unsafe.As<uint, T>(ref r); };
                    break;
                case FormatCode.UInt:
                    reader = static b => { uint r = AmqpBitConverter.ReadUInt(b); return Unsafe.As<uint, T>(ref r); };
                    break;
                case FormatCode.SmallUInt:
                    reader = static b => { uint r = AmqpBitConverter.ReadUByte(b); return Unsafe.As<uint, T>(ref r); };
                    break;
                case FormatCode.ULong0:
                    unboundedSize = FixedWidth.ULong;
                    reader = static b => { ulong r = 0; return Unsafe.As<ulong, T>(ref r); };
                    break;
                case FormatCode.ULong:
                    reader = static b => { ulong r = AmqpBitConverter.ReadULong(b); return Unsafe.As<ulong, T>(ref r); };
                    break;
                case FormatCode.SmallULong:
                    reader = static b => { ulong r = AmqpBitConverter.ReadUByte(b); return Unsafe.As<ulong, T>(ref r); };
                    break;
                case FormatCode.Byte:
                    reader = static b => { sbyte r = AmqpBitConverter.ReadByte(b); return Unsafe.As<sbyte, T>(ref r); };
                    break;
                case FormatCode.Short:
                    reader = static b => { short r = AmqpBitConverter.ReadShort(b); return Unsafe.As<short, T>(ref r); };
                    break;
                case FormatCode.Int:
                    reader = static b => { int r = AmqpBitConverter.ReadInt(b); return Unsafe.As<int, T>(ref r); };
                    break;
                case FormatCode.SmallInt:
                    reader = static b => { int r = AmqpBitConverter.ReadByte(b); return Unsafe.As<int, T>(ref r); };
                    break;
                case FormatCode.Long:
                    reader = static b => { long r = AmqpBitConverter.ReadLong(b); return Unsafe.As<long, T>(ref r); };
                    break;
                case FormatCode.SmallLong:
                    reader = static b => { long r = AmqpBitConverter.ReadByte(b); return Unsafe.As<long, T>(ref r); };
                    break;
                case FormatCode.Float:
                    reader = static b => { float r = AmqpBitConverter.ReadFloat(b); return Unsafe.As<float, T>(ref r); };
                    break;
                case FormatCode.Double:
                    reader = static b => { double r = AmqpBitConverter.ReadDouble(b); return Unsafe.As<double, T>(ref r); };
                    break;
                case FormatCode.Char:
                    reader = static b => { char r = char.ConvertFromUtf32(AmqpBitConverter.ReadInt(b))[0]; return Unsafe.As<char, T>(ref r); };
                    break;
                case FormatCode.TimeStamp:
                    reader = static b => { DateTime r = TimeStampEncoding.ToDateTime(AmqpBitConverter.ReadLong(b)); return Unsafe.As<DateTime, T>(ref r); };
                    break;
                case FormatCode.Uuid:
                    reader = static b => { Guid r = AmqpBitConverter.ReadUuid(b); return Unsafe.As<Guid, T>(ref r); };
                    break;
                case FormatCode.Symbol32:
                    reader = static b => { AmqpSymbol r = SymbolEncoding.Decode(b, FormatCode.Symbol32); return Unsafe.As<AmqpSymbol, T>(ref r); };
                    break;
                case FormatCode.Symbol8:
                    reader = static b => { AmqpSymbol r = SymbolEncoding.Decode(b, FormatCode.Symbol8); return Unsafe.As<AmqpSymbol, T>(ref r); };
                    break;
                case FormatCode.Decimal32:
                    reader = static b => { decimal r = DecimalEncoding.DecodeValue(b, FormatCode.Decimal32); return Unsafe.As<decimal, T>(ref r); };
                    break;
                case FormatCode.Decimal64:
                    reader = static b => { decimal r = DecimalEncoding.DecodeValue(b, FormatCode.Decimal64); return Unsafe.As<decimal, T>(ref r); };
                    break;
                case FormatCode.Decimal128:
                    reader = static b => { decimal r = DecimalEncoding.DecodeValue(b, FormatCode.Decimal128); return Unsafe.As<decimal, T>(ref r); };
                    break;
                case FormatCode.List0:
                    unboundedSize = IntPtr.Size;
                    break;
                default:
                    break;
            }

            return reader;
        }
#else
        /// <summary>
        /// Returns the in-memory element size for unbounded (zero-width) format codes.
        /// Returns 0 for format codes that consume buffer bytes during decode.
        /// </summary>
        static int GetUnboundedElementSize(FormatCode formatCode)
        {
            switch (formatCode)
            {
                case FormatCode.BooleanTrue:
                case FormatCode.BooleanFalse:
                    return FixedWidth.BooleanVar;
                case FormatCode.UInt0:
                    return FixedWidth.UInt;
                case FormatCode.ULong0:
                    return FixedWidth.ULong;
                case FormatCode.List0:
                    return IntPtr.Size;
                default:
                    return 0;
            }
        }
#endif

        static T[] Decode<T>(ByteBuffer buffer, int size, int count, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            int capacity = Math.Min(count, MaxInitialSize);
            T[] array = new T[capacity];
            EncodingBase encoding = AmqpEncoding.GetEncoding(formatCode);
            object descriptor = null;
            if (formatCode == FormatCode.Described)
            {
                descriptor = AmqpEncoding.DecodeObject(buffer, depth + 1, ref totalUnboundedSize);
                formatCode = AmqpEncoding.ReadFormatCode(buffer);
                encoding = AmqpEncoding.GetEncoding(formatCode);
            }

#if NET8_0_OR_GREATER
            Func<ByteBuffer, T> reader = GetReader<T>(formatCode, out int unboundedSize);

            for (int i = 0; i < count; ++i)
            {
                AmqpEncoding.TrackUnboundedSize(ref totalUnboundedSize, unboundedSize);

                if (i >= capacity)
                {
                    capacity += Math.Min(count - i, Math.Min(capacity * 2, MaxGrowFactor));
                    Array.Resize(ref array, capacity);
                }

                if (descriptor != null)
                {
                    object value = new DescribedType(descriptor, encoding.DecodeObject(buffer, formatCode, depth + 1, ref totalUnboundedSize));
                    array[i] = (T)value;
                }
                else if (reader != null)
                {
                    array[i] = reader(buffer);
                }
                else
                {
                    array[i] = (T)encoding.DecodeObject(buffer, formatCode, depth + 1, ref totalUnboundedSize);
                }
            }
#else
            {
                int unboundedSize = GetUnboundedElementSize(formatCode);

                for (int i = 0; i < count; ++i)
                {
                    if (i >= capacity)
                    {
                        capacity += Math.Min(count - i, Math.Min(capacity * 2, MaxGrowFactor));
                        Array.Resize(ref array, capacity);
                    }

                    AmqpEncoding.TrackUnboundedSize(ref totalUnboundedSize, unboundedSize);
                    object value = encoding.DecodeObject(buffer, formatCode, depth + 1, ref totalUnboundedSize);
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
