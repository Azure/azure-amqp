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
        static Dictionary<Type, IEncoding> encodingsByType;
        static Dictionary<FormatCode, IEncoding> encodingsByCode;

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
            encodingsByType = new Dictionary<Type, IEncoding>()
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

            encodingsByCode = new Dictionary<FormatCode, IEncoding>()
            {
                { FormatCode.BooleanFalse,  Boolean },
                { FormatCode.BooleanTrue,   Boolean },
                { FormatCode.Boolean,       Boolean },
                { FormatCode.UByte,         UByte },
                { FormatCode.UShort,        UShort },
                { FormatCode.UInt,          UInt },
                { FormatCode.SmallUInt,     UInt },
                { FormatCode.UInt0,         UInt },
                { FormatCode.ULong,         ULong },
                { FormatCode.SmallULong,    ULong },
                { FormatCode.ULong0,        ULong },
                { FormatCode.Byte,          Byte },
                { FormatCode.Short,         Short },
                { FormatCode.Int,           Int },
                { FormatCode.SmallInt,      Int },
                { FormatCode.Long,          Long },
                { FormatCode.SmallLong,     Long },
                { FormatCode.Float,         Float },
                { FormatCode.Double,        Double },
                { FormatCode.Decimal128,    Decimal },
                { FormatCode.Char,          Char },
                { FormatCode.TimeStamp,     Timestamp },
                { FormatCode.Uuid,          Uuid },
                { FormatCode.Binary8,       Binary },
                { FormatCode.Binary32,      Binary },
                { FormatCode.Symbol8,       Symbol },
                { FormatCode.Symbol32,      Symbol },
                { FormatCode.String8Utf8,   String },
                { FormatCode.String32Utf8,  String },
                { FormatCode.List0,         List },
                { FormatCode.List8,         List },
                { FormatCode.List32,        List },
                { FormatCode.Map8,          Map },
                { FormatCode.Map32,         Map },
                { FormatCode.Array8,        Array },
                { FormatCode.Array32,       Array },
                { FormatCode.Described,     Described }
            };
        }

        internal static IEncoding GetEncoding(FormatCode formatCode)
        {
            if (encodingsByCode.TryGetValue(formatCode, out IEncoding encoding))
            {
                return encoding;
            }

            throw new NotSupportedException(AmqpResources.GetString(AmqpResources.AmqpInvalidType, formatCode));
        }

        internal static EncodingBase<T> GetEncoding<T>()
        {
            return (EncodingBase<T>)GetEncoding(typeof(T));
        }

        internal static bool TryGetEncoding(Type type, out IEncoding encoding)
        {
            if (encodingsByType.TryGetValue(type, out encoding))
            {
                return true;
            }

            if (type.IsArray)
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

        internal static IEncoding GetEncoding(Type type)
        {
            if (TryGetEncoding(type, out IEncoding encoding))
            {
                return encoding;
            }

            throw new NotSupportedException(AmqpResources.GetString(AmqpResources.AmqpInvalidType, type.FullName));
        }

        internal static int GetEncodeWidthBySize(int size)
        {
            return size <= byte.MaxValue ? FixedWidth.UByte : FixedWidth.UInt;
        }

        internal static int GetEncodeWidthByCountAndSize(int count, int valueSize)
        {
            return count < byte.MaxValue && valueSize < byte.MaxValue ? FixedWidth.UByte : FixedWidth.UInt;
        }

        internal static FormatCode ReadFormatCode(ByteBuffer buffer)
        {
            byte type = AmqpBitConverter.ReadUByte(buffer);
            byte extType = 0;
            if (FormatCode.HasExtType(type))
            {
                extType = AmqpBitConverter.ReadUByte(buffer);
            }

            return new FormatCode(type, extType);
        }

        internal static void ReadCount(ByteBuffer buffer, FormatCode formatCode, FormatCode formatCode8, FormatCode formatCode32, out int count)
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
        }

        internal static void ReadSizeAndCount(ByteBuffer buffer, FormatCode formatCode, FormatCode formatCode8, FormatCode formatCode32, out int size, out int count)
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

            IEncoding encoding = GetEncoding(value.GetType());
            return encoding.GetSize(value);
        }

        internal static void EncodeNull(ByteBuffer buffer)
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

            if (TryGetEncoding(value.GetType(), out IEncoding encoding))
            {
                encoding.Write(value, buffer, -1);
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
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return DecodeObject(buffer, formatCode);
        }

        internal static object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            IEncoding encoding;
            if (encodingsByCode.TryGetValue(formatCode, out encoding))
            {
                return encoding.Read(buffer, formatCode);
            }

            throw GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
        }

        internal static AmqpException GetEncodingException(string message)
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
