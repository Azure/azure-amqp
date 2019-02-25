// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Target : DescribedList
    {
        public static readonly string Name = "amqp:target:list";
        public static readonly ulong Code = 0x0000000000000029;
        const int Fields = 7;

        public Target() : base(Name, Code) { }

        public Target(Uri uri) : this()
        {
            this.Address = uri.AbsoluteUri;
        }

        public Address Address { get; set; }

        public uint? Durable { get; set; }

        public AmqpSymbol ExpiryPolicy { get; set; }

        public uint? Timeout { get; set; }

        public bool? Dynamic { get; set; }

        public Fields DynamicNodeProperties { get; set; }

        public Multiple<AmqpSymbol> Capabilities { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("target(");
            int count = 0;
            this.AddFieldToString(this.Address != null, sb, "address", this.Address, ref count);
            this.AddFieldToString(this.Durable != null, sb, "durable", this.Durable, ref count);
            this.AddFieldToString(this.ExpiryPolicy.Value != null, sb, "expiry-policy", this.ExpiryPolicy, ref count);
            this.AddFieldToString(this.Timeout != null, sb, "timeout", this.Timeout, ref count);
            this.AddFieldToString(this.Dynamic != null, sb, "dynamic", this.Dynamic, ref count);
            this.AddFieldToString(this.DynamicNodeProperties != null, sb, "dynamic-node-properties", this.DynamicNodeProperties, ref count);
            this.AddFieldToString(this.Capabilities != null, sb, "capabilities", this.Capabilities, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
            Address.Encode(buffer, this.Address);
            AmqpCodec.EncodeUInt(this.Durable, buffer);
            AmqpCodec.EncodeSymbol(this.ExpiryPolicy, buffer);
            AmqpCodec.EncodeUInt(this.Timeout, buffer);
            AmqpCodec.EncodeBoolean(this.Dynamic, buffer);
            AmqpCodec.EncodeMap(this.DynamicNodeProperties, buffer);
            AmqpCodec.EncodeMultiple(this.Capabilities, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
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
                this.Capabilities = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }
        }

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += Address.GetEncodeSize(this.Address);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Durable);
            valueSize += AmqpCodec.GetSymbolEncodeSize(this.ExpiryPolicy);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Timeout);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Dynamic);
            valueSize += AmqpCodec.GetMapEncodeSize(this.DynamicNodeProperties);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.Capabilities);

            return valueSize;
        }
    }
}
