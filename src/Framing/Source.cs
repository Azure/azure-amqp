// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the source.
    /// </summary>
    public sealed class Source : DescribedList
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:source:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000028;
        const int Fields = 11;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Source() : base(Name, Code) { }

        /// <summary>
        /// Gets or sets the "address" field.
        /// </summary>
        public Address Address { get; set; }

        /// <summary>
        /// Gets or sets the "durable" field.
        /// </summary>
        public uint? Durable { get; set; }

        /// <summary>
        /// Gets or sets the "expiry-policy" field.
        /// </summary>
        public AmqpSymbol ExpiryPolicy { get; set; }

        /// <summary>
        /// Gets or sets the "timeout" field.
        /// </summary>
        public uint? Timeout { get; set; }

        /// <summary>
        /// Gets or sets the "dynamic" field.
        /// </summary>
        public bool? Dynamic { get; set; }

        /// <summary>
        /// Gets or sets the "dynamic-node-properties" field.
        /// </summary>
        public Fields DynamicNodeProperties { get; set; }

        /// <summary>
        /// Gets or sets the "distribution-mode" field.
        /// </summary>
        public AmqpSymbol DistributionMode { get; set; }

        /// <summary>
        /// Gets or sets the "filter-set" field.
        /// </summary>
        public FilterSet FilterSet { get; set; }

        /// <summary>
        /// Gets or sets the "default-outcome" field.
        /// </summary>
        public Outcome DefaultOutcome { get; set; }

        /// <summary>
        /// Gets or sets the "outcomes" field.
        /// </summary>
        public Multiple<AmqpSymbol> Outcomes { get; set; }

        /// <summary>
        /// Gets or sets the "capabilities" field.
        /// </summary>
        public Multiple<AmqpSymbol> Capabilities { get; set; }

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
            StringBuilder sb = new StringBuilder("source(");
            int count = 0;
            this.AddFieldToString(this.Address != null, sb, "address", this.Address, ref count);
            this.AddFieldToString(this.Durable != null, sb, "durable", this.Durable, ref count);
            this.AddFieldToString(this.ExpiryPolicy.Value != null, sb, "expiry-policy", this.ExpiryPolicy, ref count);
            this.AddFieldToString(this.Timeout != null, sb, "timeout", this.Timeout, ref count);
            this.AddFieldToString(this.Dynamic != null, sb, "dynamic", this.Dynamic, ref count);
            this.AddFieldToString(this.DynamicNodeProperties != null, sb, "dynamic-node-properties", this.DynamicNodeProperties, ref count);
            this.AddFieldToString(this.DistributionMode.Value != null, sb, "distribution-mode", this.DistributionMode, ref count);
            this.AddFieldToString(this.FilterSet != null, sb, "filter", this.FilterSet, ref count);
            this.AddFieldToString(this.DefaultOutcome != null, sb, "default-outcome", this.DefaultOutcome, ref count);
            this.AddFieldToString(this.Outcomes != null, sb, "outcomes", this.Outcomes, ref count);
            this.AddFieldToString(this.Capabilities != null, sb, "capabilities", this.Capabilities, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            Address.Encode(buffer, this.Address);
            AmqpCodec.EncodeUInt(this.Durable, buffer);
            AmqpCodec.EncodeSymbol(this.ExpiryPolicy, buffer);
            AmqpCodec.EncodeUInt(this.Timeout, buffer);
            AmqpCodec.EncodeBoolean(this.Dynamic, buffer);
            AmqpCodec.EncodeMap(this.DynamicNodeProperties, buffer);
            AmqpCodec.EncodeSymbol(this.DistributionMode, buffer);
            AmqpCodec.EncodeMap(this.FilterSet, buffer);
            AmqpCodec.EncodeSerializable(this.DefaultOutcome, buffer);
            AmqpCodec.EncodeMultiple(this.Outcomes, buffer);
            AmqpCodec.EncodeMultiple(this.Capabilities, buffer);
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
                this.Address = Address.Decode(buffer);
            }

            if (count-- > 0)
            {
                this.Durable = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.ExpiryPolicy = AmqpCodec.DecodeSymbol(buffer);
            }

            if (count-- > 0)
            {
                this.Timeout = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Dynamic = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.DynamicNodeProperties = AmqpCodec.DecodeMap<Fields>(buffer);
            }

            if (count-- > 0)
            {
                this.DistributionMode = AmqpCodec.DecodeSymbol(buffer);
            }

            if (count-- > 0)
            {
                this.FilterSet = AmqpCodec.DecodeMap<FilterSet>(buffer);
            }

            if (count-- > 0)
            {
                this.DefaultOutcome = (Outcome)AmqpCodec.DecodeAmqpDescribed(buffer);
            }

            if (count-- > 0)
            {
                this.Outcomes = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }

            if (count-- > 0)
            {
                this.Capabilities = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += Address.GetEncodeSize(this.Address);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Durable);
            valueSize += AmqpCodec.GetSymbolEncodeSize(this.ExpiryPolicy);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Timeout);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Dynamic);
            valueSize += AmqpCodec.GetMapEncodeSize(this.DynamicNodeProperties);
            valueSize += AmqpCodec.GetSymbolEncodeSize(this.DistributionMode);
            valueSize += AmqpCodec.GetMapEncodeSize(this.FilterSet);
            valueSize += AmqpCodec.GetSerializableEncodeSize(this.DefaultOutcome);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.Outcomes);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.Capabilities);

            return valueSize;
        }
    }
}
