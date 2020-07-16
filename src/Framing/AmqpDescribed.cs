// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Globalization;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// The base class of AMQP defined composite types where descriptor is
    /// a name (symbol) or a code (ulong).
    /// </summary>
    public class AmqpDescribed : IAmqpSerializable
    {
        AmqpSymbol name;
        ulong code;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="name">The descriptor name.</param>
        /// <param name="code">The descriptor code.</param>
        public AmqpDescribed(AmqpSymbol name, ulong code)
        {
            this.name = name;
            this.code = code;
        }

        /// <summary>
        /// Gets the descriptor name.
        /// </summary>
        public AmqpSymbol DescriptorName
        {
            get { return this.name; }
        }

        /// <summary>
        /// Gets the descriptor code.
        /// </summary>
        public ulong DescriptorCode
        {
            get { return this.code; }
        }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public object Value
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the encode size of the object.
        /// </summary>
        public int EncodeSize
        {
            get
            {
                int encodeSize = FixedWidth.FormatCode + ULongEncoding.GetEncodeSize(this.DescriptorCode);
                encodeSize += this.GetValueEncodeSize();
                return encodeSize;
            }
        }

        internal int Offset { get; set; }

        internal int Length { get; set; }

        static void DecodeDescriptor(ByteBuffer buffer, out AmqpSymbol name, out ulong code)
        {
            name = default(AmqpSymbol);
            code = 0;

            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Described)
            {
                formatCode = AmqpEncoding.ReadFormatCode(buffer);
            }

            if (formatCode == FormatCode.Symbol8 ||
                formatCode == FormatCode.Symbol32)
            {
                name = SymbolEncoding.Decode(buffer, formatCode);
            }
            else if (formatCode == FormatCode.ULong ||
                formatCode == FormatCode.ULong0 ||
                formatCode == FormatCode.SmallULong)
            {
                code = ULongEncoding.Decode(buffer, formatCode).Value;
            }
            else
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return $"{this.name}:{this.Value}";
        }

        /// <summary>
        /// Encodes the object to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        public void Encode(ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Described);
            ULongEncoding.Encode(this.DescriptorCode, buffer);
            this.EncodeValue(buffer);
        }

        /// <summary>
        /// Decodes the object from a buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        public void Decode(ByteBuffer buffer)
        {
            // TODO: validate that the name or code is actually correct
            DecodeDescriptor(buffer, out this.name, out this.code);
            this.DecodeValue(buffer);
        }

        internal virtual int GetValueEncodeSize()
        {
            return AmqpEncoding.GetObjectEncodeSize(this.Value);
        }

        internal virtual void EncodeValue(ByteBuffer buffer)
        {
            AmqpEncoding.EncodeObject(this.Value, buffer);
        }

        internal virtual void DecodeValue(ByteBuffer buffer)
        {
            this.Value = AmqpEncoding.DecodeObject(buffer);
        }

        /// <summary>
        /// Adds a field to the string builder to create a string of the container object.
        /// </summary>
        protected void AddFieldToString(bool condition, StringBuilder sb, string fieldName, object value, ref int count)
        {
            if (condition)
            {
                if (count > 0)
                {
                    sb.Append(',');
                }

                if (value is ArraySegment<byte>)
                {
                    sb.Append(fieldName);
                    sb.Append(':');
                    ArraySegment<byte> binValue = (ArraySegment<byte>)value;
                    int size = Math.Min(binValue.Count, 64);
                    for (int i = 0; i < size; ++i)
                    {
                        sb.AppendFormat(CultureInfo.InvariantCulture, "{0:X2}", binValue.Array[binValue.Offset + i]);
                    }
                }
                else
                {
                    sb.AppendFormat(CultureInfo.InvariantCulture, "{0}:{1}", fieldName, value);
                }

                ++count;
            }
        }
    }
}
