// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    /// <summary>
    /// Encodes and decodes AMQP types.
    /// </summary>
    public static class AmqpEncoding
    {
        static Dictionary<Type, EncodingBase> encodingsByType;

        internal static BooleanEncoding Boolean = new BooleanEncoding();
        internal static UByteEncoding UByte = new UByteEncoding();
        internal static UShortEncoding UShort = new UShortEncoding();
        internal static UIntEncoding UInt = new UIntEncoding();
        internal static ULongEncoding ULong = new ULongEncoding();
        internal static ByteEncoding Byte = new ByteEncoding();
        internal static ShortEncoding Short = new ShortEncoding();
        internal static IntEncoding Int = new IntEncoding();
        internal static LongEncoding Long = new LongEncoding();
        internal static FloatEncoding Float = new FloatEncoding();
        internal static DoubleEncoding Double = new DoubleEncoding();
        internal static DecimalEncoding Decimal = new DecimalEncoding();
        internal static CharEncoding Char = new CharEncoding();
        internal static TimeStampEncoding Timestamp = new TimeStampEncoding();
        internal static UuidEncoding Uuid = new UuidEncoding();
        internal static BinaryEncoding Binary = new BinaryEncoding();
        internal static SymbolEncoding Symbol = new SymbolEncoding();
        internal static StringEncoding String = new StringEncoding();
        internal static ListEncoding List = new ListEncoding();
        internal static MapEncoding Map = new MapEncoding();
        internal static ArrayEncoding Array = new ArrayEncoding();
        internal static DescribedEncoding Described = new DescribedEncoding();

        static AmqpEncoding()
        {
            encodingsByType = new Dictionary<Type, EncodingBase>()
            {
                { typeof(bool),             Boolean },
                { typeof(byte),             UByte },
                { typeof(ushort),           UShort },
                { typeof(uint),             UInt },
                { typeof(ulong),            ULong },
                { typeof(sbyte),            Byte },
                { typeof(short),            Short },
                { typeof(int),              Int },
                { typeof(long),             Long },
                { typeof(float),            Float },
                { typeof(double),           Double },
                { typeof(decimal),          Decimal },
                { typeof(char),             Char },
                { typeof(DateTime),         Timestamp },
                { typeof(Guid),             Uuid },
                { typeof(ArraySegment<byte>), Binary },
                { typeof(AmqpSymbol),       Symbol },
                { typeof(string),           String },
                { typeof(AmqpMap),          Map },
            };
        }

        /// <summary>
        /// Gets the encoding for the specified format code.
        /// </summary>
        /// <param name="formatCode">The format code.</param>
        /// <returns>The encoding.</returns>
        public static EncodingBase GetEncoding(FormatCode formatCode)
        {
            switch (formatCode.Type)
            {
                case FormatCode.Described:
                    return Described;
                case FormatCode.BooleanFalse:
                case FormatCode.BooleanTrue:
                case FormatCode.Boolean:
                    return Boolean;
                case FormatCode.UByte:
                    return UByte;
                case FormatCode.UShort:
                    return UShort;
                case FormatCode.UInt:
                case FormatCode.SmallUInt:
                case FormatCode.UInt0:
                    return UInt;
                case FormatCode.ULong:
                case FormatCode.SmallULong:
                case FormatCode.ULong0:
                    return ULong;
                case FormatCode.Byte:
                    return Byte;
                case FormatCode.Short:
                    return Short;
                case FormatCode.Int:
                case FormatCode.SmallInt:
                    return Int;
                case FormatCode.Long:
                case FormatCode.SmallLong:
                    return Long;
                case FormatCode.Float:
                    return Float;
                case FormatCode.Double:
                    return Double;
                case FormatCode.Decimal32:
                case FormatCode.Decimal64:
                case FormatCode.Decimal128:
                    return Decimal;
                case FormatCode.Char:
                    return Char;
                case FormatCode.TimeStamp:
                    return Timestamp;
                case FormatCode.Uuid:
                    return Uuid;
                case FormatCode.Binary8:
                case FormatCode.Binary32:
                    return Binary;
                case FormatCode.Symbol8:
                case FormatCode.Symbol32:
                    return Symbol;
                case FormatCode.String8Utf8:
                case FormatCode.String32Utf8:
                    return String;
                case FormatCode.List0:
                case FormatCode.List8:
                case FormatCode.List32:
                    return List;
                case FormatCode.Map8:
                case FormatCode.Map32:
                    return Map;
                case FormatCode.Array8:
                case FormatCode.Array32:
                    return Array;
                default:
                    throw new NotSupportedException(AmqpResources.GetString(AmqpResources.AmqpInvalidType, formatCode));
            }
        }

        internal static EncodingBase<T> GetEncoding<T>()
        {
            return (EncodingBase<T>)GetEncoding(typeof(T));
        }

        internal static bool TryGetEncoding(Type type, out EncodingBase encoding)
        {
            if (encodingsByType.TryGetValue(type, out encoding))
            {
                return true;
            }

            if (type.IsArray || type == typeof(Array))
            {
                encoding = Array;
                return true;
            }

            if (typeof(IList).IsAssignableFrom(type))
            {
                encoding = List;
                return true;
            }

            if (typeof(DescribedType).IsAssignableFrom(type))
            {
                encoding = Described;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Gets the encoding for the specified type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The encoding.</returns>
        public static EncodingBase GetEncoding(Type type)
        {
            if (TryGetEncoding(type, out EncodingBase encoding))
            {
                return encoding;
            }

            throw new NotSupportedException(AmqpResources.GetString(AmqpResources.AmqpInvalidType, type.FullName));
        }

        /// <summary>
        /// Gets the encoding for the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The encoding.</returns>
        public static EncodingBase GetEncoding(object value)
        {
            if (value == null)
            {
                throw new ArgumentNullException(nameof(value));
            }

            return GetEncoding(value.GetType());
        }

        /// <summary>
        /// Gets the encode width (1 or 4 bytes) based on the size.
        /// </summary>
        /// <param name="size">The size value.</param>
        /// <returns>The encode width.</returns>
        public static int GetEncodeWidthBySize(int size)
        {
            return size <= byte.MaxValue ? FixedWidth.UByte : FixedWidth.UInt;
        }

        /// <summary>
        /// Gets the encode width (1 or 4 bytes) based on count and size.
        /// </summary>
        /// <param name="count">The element count.</param>
        /// <param name="valueSize">The value size.</param>
        /// <returns>The encode width.</returns>
        public static int GetEncodeWidthByCountAndSize(int count, int valueSize)
        {
            return count < byte.MaxValue && valueSize < byte.MaxValue ? FixedWidth.UByte : FixedWidth.UInt;
        }

        /// <summary>
        /// Reads a format code from the buffer.
        /// </summary>
        /// <param name="buffer">The source buffer.</param>
        /// <returns>The format code.</returns>
        public static FormatCode ReadFormatCode(ByteBuffer buffer)
        {
            byte type = AmqpBitConverter.ReadUByte(buffer);
            byte extType = 0;
            if (FormatCode.HasExtType(type))
            {
                extType = AmqpBitConverter.ReadUByte(buffer);
            }

            return new FormatCode(type, extType);
        }

        /// <summary>
        /// Reads the count from the buffer based on the format code width.
        /// </summary>
        /// <param name="buffer">The source buffer.</param>
        /// <param name="formatCode">The actual format code.</param>
        /// <param name="formatCode8">The 1-byte width format code.</param>
        /// <param name="formatCode32">The 4-byte width format code.</param>
        /// <param name="count">The count read.</param>
        public static void ReadCount(ByteBuffer buffer, FormatCode formatCode, FormatCode formatCode8, FormatCode formatCode32, out int count)
        {
            if (formatCode == formatCode8)
            {
                count = AmqpBitConverter.ReadUByte(buffer);
            }
            else if (formatCode == formatCode32)
            {
                count = (int)AmqpBitConverter.ReadUInt(buffer);
            }
            else
            {
                throw GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            if (count < 0 || count > buffer.Length)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError,
                    string.Format("AMQP variable width {0} exceeds buffer length ({1}).", (uint)count, buffer.Length));
            }
        }

        /// <summary>
        /// Reads the size and count from the buffer based on the format code width.
        /// </summary>
        /// <param name="buffer">The source buffer.</param>
        /// <param name="formatCode">The actual format code.</param>
        /// <param name="formatCode8">The 1-byte width format code.</param>
        /// <param name="formatCode32">The 4-byte width format code.</param>
        /// <param name="size">The size read.</param>
        /// <param name="count">The count read.</param>
        public static void ReadSizeAndCount(ByteBuffer buffer, FormatCode formatCode, FormatCode formatCode8, FormatCode formatCode32, out int size, out int count)
        {
            if (formatCode == formatCode8)
            {
                size = AmqpBitConverter.ReadUByte(buffer);
                count = AmqpBitConverter.ReadUByte(buffer);
            }
            else if (formatCode == formatCode32)
            {
                size = (int)AmqpBitConverter.ReadUInt(buffer);
                count = (int)AmqpBitConverter.ReadUInt(buffer);
            }
            else
            {
                throw GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            // AMQP size includes the count field but not the size field itself.
            // After reading both, buffer.Length is the remaining data bytes.
            // Adding FixedWidth.UInt accounts for the already-consumed count field.
            if (size < 0 || size > buffer.Length + FixedWidth.UInt)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError,
                    string.Format("AMQP collection size {0} exceeds buffer length ({1}).", size, buffer.Length));
            }

            if (count < 0)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError,
                    string.Format("AMQP collection count {0} is not supported.", (uint)count));
            }
        }

        /// <summary>
        /// Maximum cumulative size (in bytes) of unbounded (zero-width) elements
        /// across a single decode call. Bounds allocations for zero-width format
        /// codes (UInt0, ULong0, List0, BooleanTrue/False) whose element count is
        /// not constrained by the buffer length.
        /// </summary>
        internal const int MaxUnboundedSize = 64 * 1024;

        /// <summary>
        /// Maximum nesting depth for compound AMQP types (described, list, map, array).
        /// Prevents stack overflow from deeply nested structures.
        /// </summary>
        internal const int MaxNestingDepth = 64;

        internal static void TrackUnboundedSize(int count, int itemUnboundedSize, int bufferLength, ref int totalUnboundedSize)
        {
            if (itemUnboundedSize > 0)
            {
                long totalSize = totalUnboundedSize + (long)count * itemUnboundedSize;
                if (totalSize > AmqpEncoding.MaxUnboundedSize)
                {
                    throw new AmqpException(AmqpErrorCode.DecodeError,
                        string.Format("Total unbounded element size ({0}) exceeds maximum allowed ({1}).",
                            totalSize, AmqpEncoding.MaxUnboundedSize));
                }

                totalUnboundedSize = (int)totalSize;
            }
            else if (count > bufferLength)
            {
                // Non-zero-width element requires >= 1 buffer byte per item.
                throw new AmqpException(AmqpErrorCode.DecodeError,
                    string.Format("AMQP array count {0} exceeds buffer length ({1}).", count, bufferLength));
            }
        }

        internal static void CheckMaxNestingDepth(int depth)
        {
            if (depth > MaxNestingDepth)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError,
                    string.Format("AMQP object graph depth {0} exceeds maximum ({1}).", depth, MaxNestingDepth));
            }
        }

        /// <summary>
        /// Gets the encoded size in bytes of an object.
        /// </summary>
        /// <param name="value">The object to be encoded.</param>
        /// <returns>The encoded size in bytes of an object.</returns>
        public static int GetObjectEncodeSize(object value)
        {
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }

            IAmqpSerializable serializable = value as IAmqpSerializable;
            if (serializable != null)
            {
                return serializable.EncodeSize;
            }

            EncodingBase encoding = GetEncoding(value.GetType());
            return encoding.GetObjectEncodeSize(value, false);
        }

        /// <summary>
        /// Encodes a null value into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        public static void EncodeNull(ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Null);
        }

        /// <summary>
        /// Encodes an object and writes the bytes to the buffer.
        /// </summary>
        /// <param name="value">The object to encode.</param>
        /// <param name="buffer">The buffer to write.</param>
        public static void EncodeObject(object value, ByteBuffer buffer)
        {
            if (value == null)
            {
                EncodeNull(buffer);
                return;
            }

            if (value is IAmqpSerializable serializable)
            {
                serializable.Encode(buffer);
                return;
            }

            if (TryGetEncoding(value.GetType(), out EncodingBase encoding))
            {
                encoding.EncodeObject(value, false, buffer);
                return;
            }

            throw new NotSupportedException(AmqpResources.GetString(AmqpResources.AmqpInvalidType, value.GetType().FullName));
        }

        /// <summary>
        /// Decodes an object from the buffer.
        /// </summary>
        /// <param name="buffer">The source buffer.</param>
        /// <returns>An object.</returns>
        public static object DecodeObject(ByteBuffer buffer)
        {
            int totalUnboundedSize = 0;
            return DecodeObject(buffer, 0, ref totalUnboundedSize);
        }

        internal static object DecodeObject(ByteBuffer buffer, int depth, ref int totalUnboundedSize)
        {
            CheckMaxNestingDepth(depth);
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return DecodeObject(buffer, formatCode, depth, ref totalUnboundedSize);
        }

        /// <summary>
        /// Decodes an object from the buffer using the specified format code.
        /// </summary>
        /// <param name="buffer">The source buffer.</param>
        /// <param name="formatCode">The format code.</param>
        /// <returns>An object.</returns>
        public static object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            int totalUnboundedSize = 0;
            return DecodeObject(buffer, formatCode, 0, ref totalUnboundedSize);
        }

        internal static object DecodeObject(ByteBuffer buffer, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            EncodingBase encoding;
            try
            {
                encoding = GetEncoding(formatCode);
            }
            catch (NotSupportedException)
            {
                throw GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            return encoding.DecodeObject(buffer, formatCode, depth, ref totalUnboundedSize);
        }

        /// <summary>
        /// Creates an encoding exception with the specified message.
        /// </summary>
        /// <param name="message">The error message.</param>
        /// <returns>An <see cref="AmqpException"/>.</returns>
        public static AmqpException GetEncodingException(string message)
        {
            return new AmqpException(AmqpErrorCode.InvalidField, message);
        }

        internal static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected)
        {
            if (formatCode != expected)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        internal static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected1, FormatCode expected2)
        {
            if (formatCode != expected1 && formatCode != expected2)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        internal static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected1, FormatCode expected2, FormatCode expected3)
        {
            if (formatCode != expected1 && formatCode != expected2 && formatCode != expected3)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        internal static void ThrowInvalidFormatCodeException(FormatCode formatCode, int offset)
        {
            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, offset));
        }
    }
}
