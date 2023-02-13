// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;

    /// <summary>
    /// Defines the header section of a message.
    /// </summary>
    public sealed class Header : DescribedList
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:header:list";
        /// <summary>Descriptor name.</summary>
        public const ulong Code = 0x0000000000000070;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Header() : base(Name, Code, 5)
        {
        }

        /// <summary>
        /// Gets or sets the "durable" field.
        /// </summary>
        public bool? Durable { get; set; }

        /// <summary>
        /// Gets or sets the "priority" field.
        /// </summary>
        public byte? Priority { get; set; }

        /// <summary>
        /// Gets or sets the "ttl" field.
        /// </summary>
        public uint? Ttl { get; set; }

        /// <summary>
        /// Gets or sets the "first-acquirer" field.
        /// </summary>
        public bool? FirstAcquirer { get; set; }

        /// <summary>
        /// Gets or sets the "delivery-count" field.
        /// </summary>
        public uint? DeliveryCount { get; set; }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("header(");
            int count = 0;
            this.AddFieldToString(this.Durable != null, sb, "durable", this.Durable, ref count);
            this.AddFieldToString(this.Priority != null, sb, "priority", this.Priority, ref count);
            this.AddFieldToString(this.Ttl != null, sb, "ttl", this.Ttl, ref count);
            this.AddFieldToString(this.FirstAcquirer != null, sb, "first-acquirer", this.FirstAcquirer, ref count);
            this.AddFieldToString(this.DeliveryCount != null, sb, "delivery-count", this.DeliveryCount, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBoolean(this.Durable, buffer);
            AmqpCodec.EncodeUByte(this.Priority, buffer);
            AmqpCodec.EncodeUInt(this.Ttl, buffer);
            AmqpCodec.EncodeBoolean(this.FirstAcquirer, buffer);
            AmqpCodec.EncodeUInt(this.DeliveryCount, buffer);
        }

        /// <summary>
        /// Decodes the fields from the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        /// <param name="count">The number of fields.</param>
        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Durable = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.Priority = AmqpCodec.DecodeUByte(buffer);
            }

            if (count-- > 0)
            {
                this.Ttl = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.FirstAcquirer = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.DeliveryCount = AmqpCodec.DecodeUInt(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize = AmqpCodec.GetBooleanEncodeSize(this.Durable);
            valueSize += AmqpCodec.GetUByteEncodeSize(this.Priority);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Ttl);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.FirstAcquirer);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.DeliveryCount);

            return valueSize;
        }
    }
}
