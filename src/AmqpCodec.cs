// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// Encodes and decodes AMQP types.
    /// </summary>
    public static class AmqpCodec
    {
        static Dictionary<string, Func<AmqpDescribed>> knownTypesByName;
        static Dictionary<ulong, Func<AmqpDescribed>> knownTypesByCode;

        static AmqpCodec()
        {
            knownTypesByName = new Dictionary<string, Func<AmqpDescribed>>()
            {
                // performatives
                { Open.Name, static () => new Open() },
                { Close.Name, static () => new Close() },
                { Begin.Name, static () => new Begin() },
                { End.Name, static () => new End() },
                { Attach.Name, static () => new Attach() },
                { Detach.Name, static () => new Detach() },
                { Transfer.Name, static () => new Transfer() },
                { Disposition.Name, static () => new Disposition() },
                { Flow.Name, static () => new Flow() },

                // transaction performatives and types
                { Coordinator.Name, static () => new Coordinator() },
                { Declare.Name, static () => new Declare() },
                { Declared.Name, static () => new Declared() },
                { Discharge.Name, static () => new Discharge() },
                { TransactionalState.Name, static () => new TransactionalState() },

                // sasl performatives
                { SaslMechanisms.Name, static () => new SaslMechanisms() },
                { SaslInit.Name, static () => new SaslInit() },
                { SaslChallenge.Name, static () => new SaslChallenge() },
                { SaslResponse.Name, static () => new SaslResponse() },
                { SaslOutcome.Name, static () => new SaslOutcome() },

                // definitions
                { Error.Name, static () => new Error() },
                { Source.Name, static () => new Source() },
                { Target.Name, static () => new Target() },
                { Received.Name, static () => new Received() },
                { Accepted.Name, static () => new Accepted() },
                { Released.Name, static () => new Released() },
                { Rejected.Name, static () => new Rejected() },
                { Modified.Name, static () => new Modified() },
                { DeleteOnClose.Name, static () => new DeleteOnClose() },
                { DeleteOnNoLinks.Name, static () => new DeleteOnNoLinks() },
                { DeleteOnNoMessages.Name, static () => new DeleteOnNoMessages() },
                { DeleteOnNoLinksOrMessages.Name, static () => new DeleteOnNoLinksOrMessages() },
            };

            knownTypesByCode = new Dictionary<ulong, Func<AmqpDescribed>>()
            {
                // frame bodies
                { Open.Code, static () => new Open() },
                { Close.Code, static () => new Close() },
                { Begin.Code, static () => new Begin() },
                { End.Code, static () => new End() },
                { Attach.Code, static () => new Attach() },
                { Detach.Code, static () => new Detach() },
                { Transfer.Code, static () => new Transfer() },
                { Disposition.Code, static () => new Disposition() },
                { Flow.Code, static () => new Flow() },

                // transaction performatives and types
                { Coordinator.Code, static () => new Coordinator() },
                { Declare.Code, static () => new Declare() },
                { Discharge.Code, static () => new Discharge() },
                { Declared.Code, static () => new Declared() },
                { TransactionalState.Code, static () => new TransactionalState() },

                // sasl frames
                { SaslMechanisms.Code, static () => new SaslMechanisms() },
                { SaslInit.Code, static () => new SaslInit() },
                { SaslChallenge.Code, static () => new SaslChallenge() },
                { SaslResponse.Code, static () => new SaslResponse() },
                { SaslOutcome.Code, static () => new SaslOutcome() },

                // definitions
                { Error.Code, static () => new Error() },
                { Source.Code, static () => new Source() },
                { Target.Code, static () => new Target() },
                { Received.Code, static () => new Received() },
                { Accepted.Code, static () => new Accepted() },
                { Released.Code, static () => new Released() },
                { Rejected.Code, static () => new Rejected() },
                { Modified.Code, static () => new Modified() },
                { DeleteOnClose.Code, static () => new DeleteOnClose() },
                { DeleteOnNoLinks.Code, static () => new DeleteOnNoLinks() },
                { DeleteOnNoMessages.Code, static () => new DeleteOnNoMessages() },
                { DeleteOnNoLinksOrMessages.Code, static () => new DeleteOnNoLinksOrMessages() },
            };
        }

        internal static int GetFrameSize(ByteBuffer buffer)
        {
            return (int)AmqpBitConverter.ReadUInt(buffer.Buffer, buffer.Offset, FixedWidth.UInt);
        }

        /// <summary>
        /// Registers a custom <see cref="AmqpDescribed"/> type to the codec for decoding.
        /// </summary>
        /// <param name="name">Descriptor name of the type.</param>
        /// <param name="code">Descriptor code of the type.</param>
        /// <param name="ctor">A function that returns an instance of the type.</param>
        public static void RegisterKnownTypes(string name, ulong code, Func<AmqpDescribed> ctor)
        {
            lock (knownTypesByCode)
            {
                if (!knownTypesByName.ContainsKey(name))
                {
                    knownTypesByName.Add(name, ctor);
                    knownTypesByCode.Add(code, ctor);
                }
            }
        }

        //// get encode size methods

        /// <summary>
        /// Gets the encode size of a boolean value.
        /// </summary>
        /// <param name="value">The boolean value.</param>
        /// <returns>Encode size in bytes of the boolean value.</returns>
        public static int GetBooleanEncodeSize(bool? value)
        {
            return value == null ? FixedWidth.NullEncoded : BooleanEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of an 8-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 8-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 8-bit unsigned integer.</returns>
        public static int GetUByteEncodeSize(byte? value)
        {
            return value == null ? FixedWidth.NullEncoded : UByteEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 16-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 16-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 16-bit unsigned integer.</returns>
        public static int GetUShortEncodeSize(ushort? value)
        {
            return value == null ? FixedWidth.NullEncoded : UShortEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 32-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 32-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 32-bit unsigned integer.</returns>
        public static int GetUIntEncodeSize(uint? value)
        {
            return value == null ? FixedWidth.NullEncoded : UIntEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 64-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 64-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 64-bit unsigned integer.</returns>
        public static int GetULongEncodeSize(ulong? value)
        {
            return value == null ? FixedWidth.NullEncoded : ULongEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of an 8-bit signed integer.
        /// </summary>
        /// <param name="value">The 8-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 8-bit signed integer.</returns>
        public static int GetByteEncodeSize(sbyte? value)
        {
            return value == null ? FixedWidth.NullEncoded : ByteEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 16-bit signed integer.
        /// </summary>
        /// <param name="value">The 16-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 16-bit signed integer.</returns>
        public static int GetShortEncodeSize(short? value)
        {
            return value == null ? FixedWidth.NullEncoded : ShortEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 32-bit signed integer.
        /// </summary>
        /// <param name="value">The 32-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 32-bit signed integer.</returns>
        public static int GetIntEncodeSize(int? value)
        {
            return value == null ? FixedWidth.NullEncoded : IntEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 64-bit signed integer.
        /// </summary>
        /// <param name="value">The 64-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 64-bit signed integer.</returns>
        public static int GetLongEncodeSize(long? value)
        {
            return value == null ? FixedWidth.NullEncoded : LongEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 32-bit floating point number.
        /// </summary>
        /// <param name="value">The 32-bit floating pointer number.</param>
        /// <returns>Encode size in bytes of the 32-bit floating pointer number.</returns>
        public static int GetFloatEncodeSize(float? value)
        {
            return value == null ? FixedWidth.NullEncoded : FloatEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a 64-bit floating point number.
        /// </summary>
        /// <param name="value">The 64-bit floating pointer number.</param>
        /// <returns>Encode size in bytes of the 64-bit floating pointer number.</returns>
        public static int GetDoubleEncodeSize(double? value)
        {
            return value == null ? FixedWidth.NullEncoded : DoubleEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a single Unicode character.
        /// </summary>
        /// <param name="value">The Unicode character.</param>
        /// <returns>Encode size in bytes of the Unicode character.</returns>
        public static int GetCharEncodeSize(char? value)
        {
            return value == null ? FixedWidth.NullEncoded : CharEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a timestamp.
        /// </summary>
        /// <param name="value">The timestamp.</param>
        /// <returns>Encode size in bytes of the timestamp.</returns>
        public static int GetTimeStampEncodeSize(DateTime? value)
        {
            return value == null ? FixedWidth.NullEncoded : TimeStampEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a uuid.
        /// </summary>
        /// <param name="value">The uuid.</param>
        /// <returns>Encode size in bytes of the uuid.</returns>
        public static int GetUuidEncodeSize(Guid? value)
        {
            return value == null ? FixedWidth.NullEncoded : UuidEncoding.GetEncodeSize(value.Value);
        }

        /// <summary>
        /// Gets the encode size of a binary value.
        /// </summary>
        /// <param name="value">The binary value.</param>
        /// <returns>Encode size in bytes of the binary value.</returns>
        public static int GetBinaryEncodeSize(ArraySegment<byte> value)
        {
            return value == null ? FixedWidth.NullEncoded : BinaryEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a symbol.
        /// </summary>
        /// <param name="value">The symbol.</param>
        /// <returns>Encode size in bytes of the symbol.</returns>
        public static int GetSymbolEncodeSize(AmqpSymbol value)
        {
            return value.Value == null ? FixedWidth.NullEncoded : SymbolEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a string.
        /// </summary>
        /// <param name="value">The string.</param>
        /// <returns>Encode size in bytes of the string.</returns>
        public static int GetStringEncodeSize(string value)
        {
            return value == null ? FixedWidth.NullEncoded : StringEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a list.
        /// </summary>
        /// <param name="value">The list.</param>
        /// <returns>Encode size in bytes of the list.</returns>
        public static int GetListEncodeSize(IList value)
        {
            return value == null ? FixedWidth.NullEncoded : ListEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a map.
        /// </summary>
        /// <param name="value">The map.</param>
        /// <returns>Encode size in bytes of the map.</returns>
        public static int GetMapEncodeSize(AmqpMap value)
        {
            return value == null ? FixedWidth.NullEncoded : MapEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of an array.
        /// </summary>
        /// <typeparam name="T">Type of items in the array.</typeparam>
        /// <param name="value">The array.</param>
        /// <returns>Encode size in bytes of the array.</returns>
        public static int GetArrayEncodeSize<T>(T[] value)
        {
            return value == null ? FixedWidth.NullEncoded : ArrayEncoding.GetEncodeSize(value, AmqpEncoding.GetEncoding<T>());
        }

        /// <summary>
        /// Gets the encode size of an <see cref="IAmqpSerializable"/> type.
        /// </summary>
        /// <param name="value">The serializable value.</param>
        /// <returns>Encode size in bytes of the serializable.</returns>
        public static int GetSerializableEncodeSize(IAmqpSerializable value)
        {
            return value == null ? FixedWidth.NullEncoded : value.EncodeSize;
        }

        /// <summary>
        /// Gets the encode size of one or multiple T objects.
        /// </summary>
        /// <typeparam name="T">Type of the object to decode.</typeparam>
        /// <param name="value">One or multiple T objects.</param>
        /// <returns>Encode size in bytes of the object.</returns>
        public static int GetMultipleEncodeSize<T>(Multiple<T> value)
        {
            return Multiple<T>.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of an AMQP object.
        /// </summary>
        /// <param name="value">The AMQP object.</param>
        /// <returns>Encode size in bytes of the AMQP object.</returns>
        public static int GetObjectEncodeSize(object value)
        {
            return AmqpEncoding.GetObjectEncodeSize(value);
        }

        //// encode methods

        /// <summary>
        /// Encodes a boolean value and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The boolean value.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeBoolean(bool? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                BooleanEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes an 8-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 8-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUByte(byte? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                UByteEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 16-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 16-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUShort(ushort? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                UShortEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 32-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 32-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUInt(uint? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                UIntEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 64-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 64-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeULong(ulong? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ULongEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes an 8-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 8-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeByte(sbyte? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ByteEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 16-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 16-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeShort(short? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ShortEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 32-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 32-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeInt(int? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                IntEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 64-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 64-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeLong(long? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                LongEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a single Unicode character and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The Unicode character.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeChar(char? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                CharEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 32-bit floating point number and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 32-bit floating point number.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeFloat(float? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                FloatEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a 64-bit floating point number and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 64-bit floating point number.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeDouble(double? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                DoubleEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a decimal number and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The decimal number.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeDecimal(decimal? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                DecimalEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a timestamp and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The timestamp.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeTimeStamp(DateTime? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                TimeStampEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a uuid and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The uuid.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUuid(Guid? data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                UuidEncoding.Encode(data.Value, buffer);
            }
        }

        /// <summary>
        /// Encodes a binary value and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The binary value.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeBinary(ArraySegment<byte> data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                BinaryEncoding.Encode(data, buffer);
            }
        }

        /// <summary>
        /// Encodes a string and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The string.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeString(string data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                StringEncoding.Encode(data, buffer);
            }
        }

        /// <summary>
        /// Encodes a symbol and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The symbol.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeSymbol(AmqpSymbol data, ByteBuffer buffer)
        {
            if (data.Value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                SymbolEncoding.Encode(data, buffer);
            }
        }

        /// <summary>
        /// Encodes a list and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The list.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeList(IList data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ListEncoding.Encode(data, buffer);
            }
        }

        /// <summary>
        /// Encodes a map and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The map.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeMap(AmqpMap data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                MapEncoding.Encode(data, buffer);
            }
        }

        /// <summary>
        /// Encodes an array and appends the bytes to the buffer.
        /// </summary>
        /// <typeparam name="T">Type of items in the array.</typeparam>
        /// <param name="data">The array.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeArray<T>(T[] data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ArrayEncoding.Encode(buffer, data, AmqpEncoding.GetEncoding<T>());
            }
        }

        /// <summary>
        /// Encodes an <see cref="IAmqpSerializable"/> object and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The serializable object.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeSerializable(IAmqpSerializable data, ByteBuffer buffer)
        {
            if (data == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                data.Encode(buffer);
            }
        }

        /// <summary>
        /// Encodes one or multiple T objects and appends the bytes to the buffer.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <param name="data">One or multiple T objects.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeMultiple<T>(Multiple<T> data, ByteBuffer buffer)
        {
            Multiple<T>.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes an AMQP object and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The AMQP object.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeObject(object data, ByteBuffer buffer)
        {
            AmqpEncoding.EncodeObject(data, buffer);
        }

        //// decode methods

        /// <summary>
        /// Decodes a boolean value from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A boolean value.</returns>
        public static bool? DecodeBoolean(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return BooleanEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes an 8-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An 8-bit unsigned integer.</returns>
        public static byte? DecodeUByte(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return UByteEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 16-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 16-bit unsigned integer.</returns>
        public static ushort? DecodeUShort(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return UShortEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 32-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 32-bit unsigned integer.</returns>
        public static uint? DecodeUInt(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return UIntEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 64-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 64-bit unsigned integer.</returns>
        public static ulong? DecodeULong(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return ULongEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes an 8-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An 8-bit signed integer.</returns>
        public static sbyte? DecodeByte(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return ByteEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 16-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 16-bit signed integer.</returns>
        public static short? DecodeShort(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return ShortEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 32-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 32-bit signed integer.</returns>
        public static int? DecodeInt(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return IntEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 64-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 64-bit signed integer.</returns>
        public static long? DecodeLong(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return LongEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 32-bit floating point number from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 32-bit floating point number.</returns>
        public static float? DecodeFloat(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return FloatEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a 64-bit floating point number from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 64-bit floating point number.</returns>
        public static double? DecodeDouble(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return DoubleEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a decimal number from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A decimal number.</returns>
        public static decimal? DecodeDecimal(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return DecimalEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a single Unicode character from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A Unicode character.</returns>
        public static char? DecodeChar(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return CharEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a timestamp from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A timestamp.</returns>
        public static DateTime? DecodeTimeStamp(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return TimeStampEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a uuid from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A uuid.</returns>
        public static Guid? DecodeUuid(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return UuidEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a binary from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A binary.</returns>
        /// <remarks>The method allocates a new buffer and copies the bytes to the returned buffer.
        /// To avoid allocation, call <see cref="BinaryEncoding.Decode(ByteBuffer, FormatCode, bool)"/>.</remarks>
        public static ArraySegment<byte> DecodeBinary(ByteBuffer buffer)
        {
            return DecodeBinary(buffer, true);
        }

        internal static ArraySegment<byte> DecodeBinary(ByteBuffer buffer, bool copy)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return default;
            }

            return BinaryEncoding.Decode(buffer, formatCode, copy);
        }

        /// <summary>
        /// Decodes a string from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A string.</returns>
        public static string DecodeString(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return StringEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a symbol from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A symbol.</returns>
        public static AmqpSymbol DecodeSymbol(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return default;
            }

            return SymbolEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a list from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A list.</returns>
        public static IList DecodeList(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return ListEncoding.Decode(buffer, formatCode);
        }

        /// <summary>
        /// Decodes a map from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A map.</returns>
        public static AmqpMap DecodeMap(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return MapEncoding.Decode(buffer, formatCode);
        }

        internal static T DecodeMap<T>(ByteBuffer buffer) where T : RestrictedMap, new()
        {
            AmqpMap map = DecodeMap(buffer);
            T restrictedMap = null;
            if (map != null)
            {
                restrictedMap = new T();
                restrictedMap.SetMap(map);
            }

            return restrictedMap;
        }

        /// <summary>
        /// Decodes an array from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An array.</returns>
        public static T[] DecodeArray<T>(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            return ArrayEncoding.Decode<T>(buffer, formatCode, AmqpEncoding.GetEncoding<T>());
        }

        /// <summary>
        /// Decodes one or a list of T from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>One or a list of T.</returns>
        public static Multiple<T> DecodeMultiple<T>(ByteBuffer buffer)
        {
            return Multiple<T>.Decode(buffer);
        }

        /// <summary>
        /// Decodes an AMQP object from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An AMQP object.</returns>
        public static object DecodeObject(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }
            else if (formatCode == FormatCode.Described)
            {
                object descriptor = AmqpCodec.DecodeObject(buffer);
                Func<AmqpDescribed> knownTypeCtor = null;
                if (descriptor is AmqpSymbol)
                {
                    knownTypesByName.TryGetValue(((AmqpSymbol)descriptor).Value, out knownTypeCtor);
                }
                else if (descriptor is ulong)
                {
                    knownTypesByCode.TryGetValue((ulong)descriptor, out knownTypeCtor);
                }

                if (knownTypeCtor != null)
                {
                    AmqpDescribed amqpDescribed = knownTypeCtor();
                    amqpDescribed.DecodeValue(buffer);
                    return amqpDescribed;
                }
                else
                {
                    object value = AmqpCodec.DecodeObject(buffer);
                    return new DescribedType(descriptor, value);
                }
            }
            else
            {
                return AmqpEncoding.DecodeObject(buffer, formatCode);
            }
        }

        /// <summary>
        /// Decodes an <see cref="AmqpDescribed"/> object from the buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>An AmqpDescribed object.</returns>
        public static AmqpDescribed DecodeAmqpDescribed(ByteBuffer buffer)
        {
            AmqpDescribed value = CreateAmqpDescribed(buffer, knownTypesByName, knownTypesByCode);
            if (value != null)
            {
                value.DecodeValue(buffer);
            }

            return value;
        }

        internal static AmqpDescribed CreateAmqpDescribed(
            ByteBuffer buffer,
            Dictionary<string, Func<AmqpDescribed>> byName,
            Dictionary<ulong, Func<AmqpDescribed>> byCode)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            AmqpEncoding.VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Described);

            Func<AmqpDescribed> knownTypeCtor = null;
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Symbol8 || formatCode == FormatCode.Symbol32)
            {
                AmqpSymbol name = SymbolEncoding.Decode(buffer, formatCode);
                byName.TryGetValue(name.Value, out knownTypeCtor);
            }
            else if (formatCode == FormatCode.ULong0 || formatCode == FormatCode.ULong || formatCode == FormatCode.SmallULong)
            {
                ulong code = ULongEncoding.Decode(buffer, formatCode);
                byCode.TryGetValue(code, out knownTypeCtor);
            }

            if (knownTypeCtor == null)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            AmqpDescribed value = knownTypeCtor();

            return value;
        }

        /// <summary>
        /// Decodes an object (AMQP and registered known types) from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An object of type T.</returns>
        public static T DecodeKnownType<T>(ByteBuffer buffer) where T : class, IAmqpSerializable, new()
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            T value = new T();
            value.Decode(buffer);
            return value;
        }
    }
}
