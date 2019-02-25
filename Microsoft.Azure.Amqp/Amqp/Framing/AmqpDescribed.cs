// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Globalization;
    using System.Text;

    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Descriptor is restricted to symbol and ulong
    /// </summary>
    public class AmqpDescribed : DescribedType, IAmqpSerializable
    {
        AmqpSymbol name;
        ulong code;

        public AmqpDescribed(AmqpSymbol name, ulong code)
            : base(name.Value == null ? (object)code : (object)name, null)
        {
            this.name = name;
            this.code = code;
        }

        public AmqpSymbol DescriptorName
        {
            get { return this.name; }
        }

        public ulong DescriptorCode
        {
            get { return this.code; }
        }

        public int EncodeSize
        {
            get
            {
                int encodeSize = FixedWidth.FormatCode + ULongEncoding.GetEncodeSize(this.DescriptorCode);
                encodeSize += this.GetValueEncodeSize();
                return encodeSize;
            }
        }

        public long Offset { get; set; }

        public long Length { get; set; }

        public static void DecodeDescriptor(ByteBuffer buffer, out AmqpSymbol name, out ulong code)
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

        public override string ToString()
        {
            return this.name.Value;
        }

        public void Encode(ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Described);
            ULongEncoding.Encode(this.DescriptorCode, buffer);
            this.EncodeValue(buffer);
        }

        public void Decode(ByteBuffer buffer)
        {
            // TODO: validate that the name or code is actually correct
            DecodeDescriptor(buffer, out this.name, out this.code);
            this.DecodeValue(buffer);
        }

        public virtual int GetValueEncodeSize()
        {
            return AmqpEncoding.GetObjectEncodeSize(this.Value);
        }

        public virtual void EncodeValue(ByteBuffer buffer)
        {
            AmqpEncoding.EncodeObject(this.Value, buffer);
        }

        public virtual void DecodeValue(ByteBuffer buffer)
        {
            this.Value = AmqpEncoding.DecodeObject(buffer);
        }

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
