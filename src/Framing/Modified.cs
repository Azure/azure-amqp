// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;

    /// <summary>
    /// Defines the modified outcome.
    /// </summary>
    public sealed class Modified : Outcome
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:modified:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000027;
        const int Fields = 3;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Modified() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the "delivery-field" field.
        /// </summary>
        public bool? DeliveryFailed { get; set; }

        /// <summary>
        /// Gets or sets the "undeliverable-here" field.
        /// </summary>
        public bool? UndeliverableHere { get; set; }

        /// <summary>
        /// Gets or sets the "message-annotations" field.
        /// </summary>
        public Fields MessageAnnotations { get; set; }

        /// <summary>
        /// Gets the number of fields in the list.
        /// </summary>
        protected override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("modified(");
            int count = 0;
            this.AddFieldToString(this.DeliveryFailed != null, sb, "delivery-failed", this.DeliveryFailed, ref count);
            this.AddFieldToString(this.UndeliverableHere != null, sb, "undeliverable-here", this.UndeliverableHere, ref count);
            this.AddFieldToString(this.MessageAnnotations != null, sb, "message-annotations", this.MessageAnnotations, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBoolean(this.DeliveryFailed, buffer);
            AmqpCodec.EncodeBoolean(this.UndeliverableHere, buffer);
            AmqpCodec.EncodeMap(this.MessageAnnotations, buffer);
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
                this.DeliveryFailed = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.UndeliverableHere = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.MessageAnnotations = AmqpCodec.DecodeMap<Fields>(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetBooleanEncodeSize(this.DeliveryFailed);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.UndeliverableHere);
            valueSize += AmqpCodec.GetMapEncodeSize(this.MessageAnnotations);

            return valueSize;
        }
    }
}
