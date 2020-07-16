// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines a composite type whose value is a list.
    /// </summary>
    public abstract class DescribedList : AmqpDescribed
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="name">The descriptor name.</param>
        /// <param name="code">The descriptor code.</param>
        public DescribedList(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }

        /// <summary>
        /// Gets the number of fields of the list.
        /// </summary>
        protected abstract int FieldCount
        {
            get;
        }

        internal override int GetValueEncodeSize()
        {
            if (this.FieldCount == 0)
            {
                return FixedWidth.FormatCode;
            }

            int valueSize = this.OnValueSize();
            int width = AmqpEncoding.GetEncodeWidthByCountAndSize(this.FieldCount, valueSize);
            return FixedWidth.FormatCode + width + width + valueSize;
        }

        internal override void EncodeValue(ByteBuffer buffer)
        {
            if (this.FieldCount == 0)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.List0);
            }
            else
            {
                int valueSize = this.OnValueSize();
                int encodeWidth = AmqpEncoding.GetEncodeWidthByCountAndSize(this.FieldCount, valueSize);
                int sizeOffset;
                if (encodeWidth == FixedWidth.UByte)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.List8);
                    sizeOffset = buffer.Length;
                    AmqpBitConverter.WriteUByte(buffer, 0);
                    AmqpBitConverter.WriteUByte(buffer, (byte)this.FieldCount);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.List32);
                    sizeOffset = buffer.Length;
                    AmqpBitConverter.WriteUInt(buffer, 0);
                    AmqpBitConverter.WriteUInt(buffer, (uint)this.FieldCount);
                }

                this.OnEncode(buffer);

                // the actual encoded value size may be different from the calculated
                // valueSize. However, it can only become smaller. This allows for
                // reserving space in the buffer using the longest encoding form of a 
                // value. For example, if the delivery id of a transfer is unknown, we
                // can use uint.Max for calculating encode size, but the actual encoding
                // could be small uint.
                int size = buffer.Length - sizeOffset - encodeWidth;
                if (encodeWidth == FixedWidth.UByte)
                {
                    AmqpBitConverter.WriteUByte(buffer.Buffer, sizeOffset, (byte)size);
                }
                else
                {
                    AmqpBitConverter.WriteUInt(buffer.Buffer, sizeOffset, (uint)size);
                }
            }
        }

        internal override void DecodeValue(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.List0)
            {
                return;
            }

            int size = 0;
            int count = 0;
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.List8, FormatCode.List32, out size, out count);
            
            int offset = buffer.Offset;
            this.DecodeValue(buffer, size, count);

            int extraCount = count - this.FieldCount;
            if (extraCount > 0)
            {
                // we just ignore the rest of bytes. ideally we should decode the remaining objects
                // to validate the buffer contains valid AMQP objects.
                int bytesRemaining = size - (buffer.Offset - offset) - (formatCode == FormatCode.List8 ? FixedWidth.UByte : FixedWidth.UInt);
                buffer.Complete(bytesRemaining);
            }
        }

        internal void DecodeValue(ByteBuffer buffer, int size, int count)
        {
            if (count > 0)
            {
                this.OnDecode(buffer, count);
                this.EnsureRequired();
            }
        }

        internal virtual void EnsureRequired()
        {
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected abstract int OnValueSize();

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected abstract void OnEncode(ByteBuffer buffer);

        /// <summary>
        /// Decodes the fields from the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        /// <param name="count">The number of fields.</param>
        protected abstract void OnDecode(ByteBuffer buffer, int count);
    }
}
