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

    interface IEncoding
    {
        FormatCode FormatCode { get; }

        // if arrayIndex >= 0, it is encoding an item in the array
        int GetSize(object obj, int arrayIndex = -1);
        void Write(object obj, ByteBuffer buffer, int arrayIndex = -1);
        object Read(ByteBuffer buffer, FormatCode formatCode);

        int GetArraySize(Array array);
        void WriteArray(Array array, ByteBuffer buffer);
        Array ReadArray(ByteBuffer buffer, FormatCode formatCode, int count);
    }

    /// <summary>
    /// Encodes and decodes amqp types.
    /// </summary>
    /// <remarks>This should be used by AmqpCodec only, where null values are handled.</remarks>
    abstract class EncodingBase<T> : IEncoding
    {
        readonly FormatCode formatCode;
        readonly int width; // -1 for varible and compact-enalbed types

        protected EncodingBase(FormatCode formatCode, int width = -1)
        {
            this.formatCode = formatCode;
            this.width = width;
        }

        public FormatCode FormatCode 
        {
            get { return this.formatCode; }
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

        int IEncoding.GetSize(object obj, int arrayIndex)
        {
            Debug.Assert(obj != null);
            return this.GetSize((T)obj, arrayIndex);
        }

        void IEncoding.Write(object obj, ByteBuffer buffer, int arrayIndex)
        {
            Debug.Assert(obj != null);
            this.Write((T)obj, buffer, arrayIndex);
        }

        object IEncoding.Read(ByteBuffer buffer, FormatCode formatCode)
        {
            Debug.Assert(formatCode != FormatCode.Null);
            return this.Read(buffer, formatCode);
        }

        int IEncoding.GetArraySize(Array array)
        {
            return ArrayEncoding.GetEncodeSize((T[])array, this);
        }

        void IEncoding.WriteArray(Array array, ByteBuffer buffer)
        {
            ArrayEncoding.Encode(buffer, (T[])array, this);
        }

        Array IEncoding.ReadArray(ByteBuffer buffer, FormatCode formatCode, int count)
        {
            return this.ReadArrayValue(buffer, formatCode, new T[count]);
        }
    }
}
