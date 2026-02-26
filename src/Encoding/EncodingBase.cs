// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Diagnostics;

    /// <summary>
    /// Implements the AMQP type system.
    /// </summary>
    /// 
    /// Notes for encoding design and implementation.
    /// * Each AMQP type implements EncodingBase of T.
    /// * The encoding methods should be in static methods so that they
    ///   can be called directly when the type is known and probably inlined.
    /// * Encoding implementations do not deal with null values, except
    ///   for those struct types (e.g. AmqpSymbol) which cannot be null
    ///   checked when the value is an object. Null check is handed in
    ///   AmqpCodec. AmqpCodec should be used always except when the type
    ///   is known and the value is not null.
    /// * Encoding methods should allow custom type to encode any types
    ///   (e.g. IList in Multiple) as arrays. The array specific methods
    ///   allow fast encoding of large arrays.
    /// * When a type is not known, AmqpEncoding can lookup the encoding by
    ///   Type or FormatCode. The IEncoding implementations handle encoding
    ///   of an object efficiently because they know the concrete type.
    /// * GetEncodeSize does not have to be exact but it must be greater
    ///   than the actual size and be as close as possible.
    ///   

    /// <summary>
    /// Base class for AMQP type encoding.
    /// </summary>
    public abstract class EncodingBase
    {
        readonly FormatCode formatCode;

        /// <summary>Initializes a new instance.</summary>
        protected EncodingBase(FormatCode formatCode)
        {
            this.formatCode = formatCode;
        }

        /// <summary>Gets the format code.</summary>
        public FormatCode FormatCode
        {
            get { return this.formatCode; }
        }

        /// <summary>Gets the encoded size of an object.</summary>
        public abstract int GetObjectEncodeSize(object value, bool arrayEncoding);

        /// <summary>Encodes an object to the buffer.</summary>
        public abstract void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer);

        /// <summary>Decodes an object from the buffer.</summary>
        public abstract object DecodeObject(ByteBuffer buffer, FormatCode formatCode);

        internal abstract Array DecodeArray(ByteBuffer buffer, FormatCode formatCode, int count);

        internal abstract int GetArrayEncodeSize(Array value);

        internal abstract void EncodeArray(Array value, ByteBuffer buffer);

        /// <summary>Verifies the format code matches the expected value.</summary>
        public static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected)
        {
            if (formatCode != expected)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        /// <summary>Verifies the format code matches one of two expected values.</summary>
        public static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected1, FormatCode expected2)
        {
            if (formatCode != expected1 && formatCode != expected2)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        /// <summary>Verifies the format code matches one of three expected values.</summary>
        public static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected1, FormatCode expected2, FormatCode expected3)
        {
            if (formatCode != expected1 && formatCode != expected2 && formatCode != expected3)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        static void ThrowInvalidFormatCodeException(FormatCode formatCode, int offset)
        {
            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, offset));
        }
    }

    /// <summary>
    /// Encodes and decodes amqp types.
    /// </summary>
    /// <remarks>This should be used by AmqpCodec only, where null values are handled.</remarks>
    abstract class EncodingBase<T> : EncodingBase
    {
        readonly int width; // -1 for variable and compact-enabled types

        protected EncodingBase(FormatCode formatCode, int width = -1)
            : base(formatCode)
        {
            this.width = width;
        }

        public int GetSize(T value, int arrayIndex = -1)
        {
            return this.width > 0 ? FixedWidth.FormatCode + this.width : this.OnGetSize(value, arrayIndex);
        }

        public void Write(T value, ByteBuffer buffer, int arrayIndex = -1)
        {
            this.OnWrite(value, buffer, arrayIndex);
        }

        public T Read(ByteBuffer buffer, FormatCode formatCode)
        {
            return this.OnRead(buffer, formatCode);
        }

        // Provide default implementations. Override them if possible for better performance.

        public virtual int GetArrayValueSize(T[] array)
        {
            if (this.width > 0)
            {
                return this.width * array.Length;
            }

            int size = 0;
            for (int i = 0; i < array.Length; i++)
            {
                size += this.GetSize(array[i], i);
            }

            return size;
        }

        public virtual void WriteArrayValue(T[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                this.Write(array[i], buffer, i);
            }
        }

        public virtual T[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, T[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = this.Read(buffer, formatCode);
            }

            return array;
        }

        protected abstract int OnGetSize(T value, int arrayIndex);

        protected abstract void OnWrite(T value, ByteBuffer buffer, int arrayIndex);

        protected abstract T OnRead(ByteBuffer buffer, FormatCode formatCode);

        /// <inheritdoc/>
        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return this.GetSize((T)value, arrayEncoding ? 0 : -1);
        }

        /// <inheritdoc/>
        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            this.Write((T)value, buffer, arrayEncoding ? 0 : -1);
        }

        /// <inheritdoc/>
        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == Encoding.FormatCode.Null)
            {
                return null;
            }

            return this.Read(buffer, formatCode);
        }

        internal override Array DecodeArray(ByteBuffer buffer, FormatCode formatCode, int count)
        {
            T[] array = new T[count];
            if (count > 0)
            {
                array = this.ReadArrayValue(buffer, formatCode, array);
            }

            return array;
        }

        internal override int GetArrayEncodeSize(Array value)
        {
            return this.GetArrayValueSize((T[])value);
        }

        internal override void EncodeArray(Array value, ByteBuffer buffer)
        {
            this.WriteArrayValue((T[])value, buffer);
        }
    }
}
