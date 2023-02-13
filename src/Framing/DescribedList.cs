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
        readonly int fieldCount;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="name">The descriptor name.</param>
        /// <param name="code">The descriptor code.</param>
        /// <param name="fieldCount">The number of fields of the list.</param>
        protected DescribedList(AmqpSymbol name, ulong code, int fieldCount)
            : base(name, code)
        {
            this.fieldCount = fieldCount;
        }

        /// <summary>
        /// Gets the number of fields of the list.
        /// </summary>
        public int FieldCount { get { return this.fieldCount; } }

        internal override int GetValueEncodeSize()
        {
            if (this.FieldCount == 0)
            {
                return FixedWidth.FormatCode;
            }

            int valueSize = this.OnValueSize();
            return FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int + valueSize;
        }

        internal override void EncodeValue(ByteBuffer buffer)
        {
            int fieldCount = this.FieldCount;
            if (fieldCount == 0)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.List0);
            }
            else
            {
                var tracker = SizeTracker.Track(buffer);
                AmqpBitConverter.WriteUByte(buffer, FormatCode.List32);
                AmqpBitConverter.WriteUInt(buffer, FixedWidth.UInt);
                AmqpBitConverter.WriteUInt(buffer, (uint)fieldCount);
                this.OnEncode(buffer);
                int size = tracker.Length - 9;
                if (size < byte.MaxValue && fieldCount <= byte.MaxValue)
                {
                    tracker.Compact(FormatCode.List8, (byte)(size + 1), (byte)fieldCount, 9);
                }
                else
                {
                    tracker.CommitExclusive(FixedWidth.FormatCode);
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

            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.List8, FormatCode.List32, out int size, out int count);
            
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
