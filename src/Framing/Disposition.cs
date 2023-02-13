// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the disposition performative.
    /// </summary>
    public sealed class Disposition : Performative
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:disposition:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000015;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Disposition() : base(Name, Code, 6)
        {
        }

        /// <summary>
        /// Gets or sets the "role" field.
        /// </summary>
        public bool? Role { get; set; }

        /// <summary>
        /// Gets or sets the "first" field.
        /// </summary>
        public uint? First { get; set; }

        /// <summary>
        /// Gets or sets the "last" field.
        /// </summary>
        public uint? Last { get; set; }

        /// <summary>
        /// Gets or sets the "settled" field.
        /// </summary>
        public bool? Settled { get; set; }

        /// <summary>
        /// Gets or sets the "state" field.
        /// </summary>
        public DeliveryState State { get; set; }

        /// <summary>
        /// Gets or sets the "batchable" field.
        /// </summary>
        public bool? Batchable { get; set; }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("disposition(");
            int count = 0;
            this.AddFieldToString(this.Role != null, sb, "role", this.Role, ref count);
            this.AddFieldToString(this.First != null, sb, "first", this.First, ref count);
            this.AddFieldToString(this.Last != null, sb, "last", this.Last, ref count);
            this.AddFieldToString(this.Settled != null, sb, "settled", this.Settled, ref count);
            this.AddFieldToString(this.State != null, sb, "state", this.State, ref count);
            this.AddFieldToString(this.Batchable != null, sb, "batchable", this.Batchable, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (!this.Role.HasValue)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "role", Name));
            }

            if (!this.First.HasValue)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "first", Name));
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBoolean(this.Role, buffer);
            AmqpCodec.EncodeUInt(this.First, buffer);
            AmqpCodec.EncodeUInt(this.Last, buffer);
            AmqpCodec.EncodeBoolean(this.Settled, buffer);
            AmqpCodec.EncodeSerializable(this.State, buffer);
            AmqpCodec.EncodeBoolean(this.Batchable, buffer);
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
                this.Role = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.First = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Last = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Settled = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.State = (DeliveryState)AmqpCodec.DecodeAmqpDescribed(buffer);
            }

            if (count-- > 0)
            {
                this.Batchable = AmqpCodec.DecodeBoolean(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Role);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.First);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Last);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Settled);
            valueSize += AmqpCodec.GetSerializableEncodeSize(this.State);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Batchable);

            return valueSize;
        }
    }
}
