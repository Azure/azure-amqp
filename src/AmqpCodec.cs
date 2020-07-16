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
                { Open.Name, () => new Open() },
                { Close.Name, () => new Close() },
                { Begin.Name, () => new Begin() },
                { End.Name, () => new End() },
                { Attach.Name, () => new Attach() },
                { Detach.Name, () => new Detach() },
                { Transfer.Name, () => new Transfer() },
                { Disposition.Name, () => new Disposition() },
                { Flow.Name, () => new Flow() },

                // transaction performatives and types
                { Coordinator.Name, () => new Coordinator() },
                { Declare.Name, () => new Declare() },
                { Declared.Name, () => new Declared() },
                { Discharge.Name, () => new Discharge() },
                { TransactionalState.Name, () => new TransactionalState() },

                // sasl performatives
                { SaslMechanisms.Name, () => new SaslMechanisms() },
                { SaslInit.Name, () => new SaslInit() },
                { SaslChallenge.Name, () => new SaslChallenge() },
                { SaslResponse.Name, () => new SaslResponse() },
                { SaslOutcome.Name, () => new SaslOutcome() },

                // definitions
                { Error.Name, () => new Error() },
                { Source.Name, () => new Source() },
                { Target.Name, () => new Target() },
                { Received.Name, () => new Received() },
                { Accepted.Name, () => new Accepted() },
                { Released.Name, () => new Released() },
                { Rejected.Name, () => new Rejected() },
                { Modified.Name, () => new Modified() },
                { DeleteOnClose.Name, () => new DeleteOnClose() },
                { DeleteOnNoLinks.Name, () => new DeleteOnNoLinks() },
                { DeleteOnNoMessages.Name, () => new DeleteOnNoMessages() },
                { DeleteOnNoLinksOrMessages.Name, () => new DeleteOnNoLinksOrMessages() },
            };

            knownTypesByCode = new Dictionary<ulong, Func<AmqpDescribed>>()
            {
                // frame bodies
                { Open.Code, () => new Open() },
                { Close.Code, () => new Close() },
                { Begin.Code, () => new Begin() },
                { End.Code, () => new End() },
                { Attach.Code, () => new Attach() },
                { Detach.Code, () => new Detach() },
                { Transfer.Code, () => new Transfer() },
                { Disposition.Code, () => new Disposition() },
                { Flow.Code, () => new Flow() },

                // transaction performatives and types
                { Coordinator.Code, () => new Coordinator() },
                { Declare.Code, () => new Declare() },
                { Discharge.Code, () => new Discharge() },
                { Declared.Code, () => new Declared() },
                { TransactionalState.Code, () => new TransactionalState() },

                // sasl frames
                { SaslMechanisms.Code, () => new SaslMechanisms() },
                { SaslInit.Code, () => new SaslInit() },
                { SaslChallenge.Code, () => new SaslChallenge() },
                { SaslResponse.Code, () => new SaslResponse() },
                { SaslOutcome.Code, () => new SaslOutcome() },

                // definitions
                { Error.Code, () => new Error() },
                { Source.Code, () => new Source() },
                { Target.Code, () => new Target() },
                { Received.Code, () => new Received() },
                { Accepted.Code, () => new Accepted() },
                { Released.Code, () => new Released() },
                { Rejected.Code, () => new Rejected() },
                { Modified.Code, () => new Modified() },
                { DeleteOnClose.Code, () => new DeleteOnClose() },
                { DeleteOnNoLinks.Code, () => new DeleteOnNoLinks() },
                { DeleteOnNoMessages.Code, () => new DeleteOnNoMessages() },
                { DeleteOnNoLinksOrMessages.Code, () => new DeleteOnNoLinksOrMessages() },
            };
        }

        internal static int GetFrameSize(ByteBuffer buffer)
        {
            return (int)AmqpBitConverter.PeekUInt(buffer);
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
            return BooleanEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of an 8-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 8-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 8-bit unsigned integer.</returns>
        public static int GetUByteEncodeSize(byte? value)
        {
            return UByteEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 16-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 16-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 16-bit unsigned integer.</returns>
        public static int GetUShortEncodeSize(ushort? value)
        {
            return UShortEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 32-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 32-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 32-bit unsigned integer.</returns>
        public static int GetUIntEncodeSize(uint? value)
        {
            return UIntEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 64-bit unsigned integer.
        /// </summary>
        /// <param name="value">The 64-bit unsigned integer.</param>
        /// <returns>Encode size in bytes of the 64-bit unsigned integer.</returns>
        public static int GetULongEncodeSize(ulong? value)
        {
            return ULongEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of an 8-bit signed integer.
        /// </summary>
        /// <param name="value">The 8-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 8-bit signed integer.</returns>
        public static int GetByteEncodeSize(sbyte? value)
        {
            return ByteEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 16-bit signed integer.
        /// </summary>
        /// <param name="value">The 16-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 16-bit signed integer.</returns>
        public static int GetShortEncodeSize(short? value)
        {
            return ShortEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 32-bit signed integer.
        /// </summary>
        /// <param name="value">The 32-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 32-bit signed integer.</returns>
        public static int GetIntEncodeSize(int? value)
        {
            return IntEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 64-bit signed integer.
        /// </summary>
        /// <param name="value">The 64-bit signed integer.</param>
        /// <returns>Encode size in bytes of the 64-bit signed integer.</returns>
        public static int GetLongEncodeSize(long? value)
        {
            return LongEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 32-bit floating point number.
        /// </summary>
        /// <param name="value">The 32-bit floating pointer number.</param>
        /// <returns>Encode size in bytes of the 32-bit floating pointer number.</returns>
        public static int GetFloatEncodeSize(float? value)
        {
            return FloatEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a 64-bit floating point number.
        /// </summary>
        /// <param name="value">The 64-bit floating pointer number.</param>
        /// <returns>Encode size in bytes of the 64-bit floating pointer number.</returns>
        public static int GetDoubleEncodeSize(double? value)
        {
            return DoubleEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a single Unicode character.
        /// </summary>
        /// <param name="value">The Unicode character.</param>
        /// <returns>Encode size in bytes of the Unicode character.</returns>
        public static int GetCharEncodeSize(char? value)
        {
            return CharEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a timestamp.
        /// </summary>
        /// <param name="value">The timestamp.</param>
        /// <returns>Encode size in bytes of the timestamp.</returns>
        public static int GetTimeStampEncodeSize(DateTime? value)
        {
            return TimeStampEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a uuid.
        /// </summary>
        /// <param name="value">The uuid.</param>
        /// <returns>Encode size in bytes of the uuid.</returns>
        public static int GetUuidEncodeSize(Guid? value)
        {
            return UuidEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a binary value.
        /// </summary>
        /// <param name="value">The binary value.</param>
        /// <returns>Encode size in bytes of the binary value.</returns>
        public static int GetBinaryEncodeSize(ArraySegment<byte> value)
        {
            return BinaryEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a symbol.
        /// </summary>
        /// <param name="value">The symbol.</param>
        /// <returns>Encode size in bytes of the symbol.</returns>
        public static int GetSymbolEncodeSize(AmqpSymbol value)
        {
            return SymbolEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a string.
        /// </summary>
        /// <param name="value">The string.</param>
        /// <returns>Encode size in bytes of the string.</returns>
        public static int GetStringEncodeSize(string value)
        {
            return StringEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a list.
        /// </summary>
        /// <param name="value">The list.</param>
        /// <returns>Encode size in bytes of the list.</returns>
        public static int GetListEncodeSize(IList value)
        {
            return ListEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of a map.
        /// </summary>
        /// <param name="value">The map.</param>
        /// <returns>Encode size in bytes of the map.</returns>
        public static int GetMapEncodeSize(AmqpMap value)
        {
            return MapEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of an array.
        /// </summary>
        /// <typeparam name="T">Type of items in the array.</typeparam>
        /// <param name="value">The array.</param>
        /// <returns>Encode size in bytes of the array.</returns>
        public static int GetArrayEncodeSize<T>(T[] value)
        {
            return ArrayEncoding.GetEncodeSize(value);
        }

        /// <summary>
        /// Gets the encode size of an <see cref="IAmqpSerializable"/> type.
        /// </summary>
        /// <param name="value">The serializable value.</param>
        /// <returns>Encode size in bytes of the serializable.</returns>
        public static int GetSerializableEncodeSize(IAmqpSerializable value)
        {
            if (value == null)
            {
                return FixedWidth.NullEncoded;
            }
            else
            {
                return value.EncodeSize;
            }
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
            BooleanEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes an 8-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 8-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUByte(byte? data, ByteBuffer buffer)
        {
            UByteEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 16-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 16-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUShort(ushort? data, ByteBuffer buffer)
        {
            UShortEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 32-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 32-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUInt(uint? data, ByteBuffer buffer)
        {
            UIntEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 64-bit unsigned integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 64-bit unsigned integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeULong(ulong? data, ByteBuffer buffer)
        {
            ULongEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes an 8-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 8-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeByte(sbyte? data, ByteBuffer buffer)
        {
            ByteEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 16-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 16-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeShort(short? data, ByteBuffer buffer)
        {
            ShortEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 32-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 32-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeInt(int? data, ByteBuffer buffer)
        {
            IntEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 64-bit signed integer and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 64-bit signed integer.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeLong(long? data, ByteBuffer buffer)
        {
            LongEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a single Unicode character and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The Unicode character.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeChar(char? data, ByteBuffer buffer)
        {
            CharEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 32-bit floating point number and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 32-bit floating point number.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeFloat(float? data, ByteBuffer buffer)
        {
            FloatEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a 64-bit floating point number and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The 64-bit floating point number.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeDouble(double? data, ByteBuffer buffer)
        {
            DoubleEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a decimal number and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The decimal number.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeDecimal(decimal? data, ByteBuffer buffer)
        {
            DecimalEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a timestamp and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The timestamp.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeTimeStamp(DateTime? data, ByteBuffer buffer)
        {
            TimeStampEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a uuid and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The uuid.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeUuid(Guid? data, ByteBuffer buffer)
        {
            UuidEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a binary value and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The binary value.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeBinary(ArraySegment<byte> data, ByteBuffer buffer)
        {
            BinaryEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a string and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The string.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeString(string data, ByteBuffer buffer)
        {
            StringEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a symbol and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The symbol.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeSymbol(AmqpSymbol data, ByteBuffer buffer)
        {
            SymbolEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a list and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The list.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeList(IList data, ByteBuffer buffer)
        {
            ListEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes a map and appends the bytes to the buffer.
        /// </summary>
        /// <param name="data">The map.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeMap(AmqpMap data, ByteBuffer buffer)
        {
            MapEncoding.Encode(data, buffer);
        }

        /// <summary>
        /// Encodes an array and appends the bytes to the buffer.
        /// </summary>
        /// <typeparam name="T">Type of items in the array.</typeparam>
        /// <param name="data">The array.</param>
        /// <param name="buffer">The destination buffer.</param>
        public static void EncodeArray<T>(T[] data, ByteBuffer buffer)
        {
            ArrayEncoding.Encode(data, buffer);
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
            return BooleanEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes an 8-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An 8-bit unsigned integer.</returns>
        public static byte? DecodeUByte(ByteBuffer buffer)
        {
            return UByteEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 16-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 16-bit unsigned integer.</returns>
        public static ushort? DecodeUShort(ByteBuffer buffer)
        {
            return UShortEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 32-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 32-bit unsigned integer.</returns>
        public static uint? DecodeUInt(ByteBuffer buffer)
        {
            return UIntEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 64-bit unsigned integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 64-bit unsigned integer.</returns>
        public static ulong? DecodeULong(ByteBuffer buffer)
        {
            return ULongEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes an 8-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>An 8-bit signed integer.</returns>
        public static sbyte? DecodeByte(ByteBuffer buffer)
        {
            return ByteEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 16-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 16-bit signed integer.</returns>
        public static short? DecodeShort(ByteBuffer buffer)
        {
            return ShortEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 32-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 32-bit signed integer.</returns>
        public static int? DecodeInt(ByteBuffer buffer)
        {
            return IntEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 64-bit signed integer from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 64-bit signed integer.</returns>
        public static long? DecodeLong(ByteBuffer buffer)
        {
            return LongEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 32-bit floating point number from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 32-bit floating point number.</returns>
        public static float? DecodeFloat(ByteBuffer buffer)
        {
            return FloatEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a 64-bit floating point number from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A 64-bit floating point number.</returns>
        public static double? DecodeDouble(ByteBuffer buffer)
        {
            return DoubleEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a decimal number from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A decimal number.</returns>
        public static decimal? DecodeDecimal(ByteBuffer buffer)
        {
            return DecimalEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a single Unicode character from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A Unicode character.</returns>
        public static char? DecodeChar(ByteBuffer buffer)
        {
            return CharEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a timestamp from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A timestamp.</returns>
        public static DateTime? DecodeTimeStamp(ByteBuffer buffer)
        {
            return TimeStampEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a uuid from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A uuid.</returns>
        public static Guid? DecodeUuid(ByteBuffer buffer)
        {
            return UuidEncoding.Decode(buffer, 0);
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
            return BinaryEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a string from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A string.</returns>
        public static string DecodeString(ByteBuffer buffer)
        {
            return StringEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a symbol from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A symbol.</returns>
        public static AmqpSymbol DecodeSymbol(ByteBuffer buffer)
        {
            return SymbolEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a list from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A list.</returns>
        public static IList DecodeList(ByteBuffer buffer)
        {
            return ListEncoding.Decode(buffer, 0);
        }

        /// <summary>
        /// Decodes a map from the buffer and advances the buffer's position.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <returns>A map.</returns>
        public static AmqpMap DecodeMap(ByteBuffer buffer)
        {
            return MapEncoding.Decode(buffer, 0);
        }

        internal static T DecodeMap<T>(ByteBuffer buffer) where T : RestrictedMap, new()
        {
            AmqpMap map = MapEncoding.Decode(buffer, 0);
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
            return ArrayEncoding.Decode<T>(buffer, 0);
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

            EncodingBase.VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Described);

            Func<AmqpDescribed> knownTypeCtor = null;
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Symbol8 || formatCode == FormatCode.Symbol32)
            {
                AmqpSymbol name = SymbolEncoding.Decode(buffer, formatCode);
                byName.TryGetValue(name.Value, out knownTypeCtor);
            }
            else if (formatCode == FormatCode.ULong0 || formatCode == FormatCode.ULong || formatCode == FormatCode.SmallULong)
            {
                ulong code = ULongEncoding.Decode(buffer, formatCode).Value;
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
