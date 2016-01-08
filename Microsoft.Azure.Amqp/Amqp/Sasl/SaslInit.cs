// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    public sealed class SaslInit : Performative
    {
        public static readonly string Name = "amqp:sasl-init:list";
        public static readonly ulong Code = 0x0000000000000041;
        const int Fields = 3;

        public SaslInit() : base(Name, Code) { }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        public AmqpSymbol Mechanism { get; set; }

        public ArraySegment<byte> InitialResponse { get; set; }

        public string HostName { get; set; }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("sasl-init(");
            int count = 0;
            this.AddFieldToString(this.Mechanism.Value != null, sb, "mechanism", this.Mechanism, ref count);
            this.AddFieldToString(this.InitialResponse.Array != null, sb, "initial-response", this.InitialResponse, ref count);
            this.AddFieldToString(this.HostName != null, sb, "host-name", this.HostName, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        protected override void EnsureRequired()
        {
            if (this.Mechanism.Value == null)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "mechanism");
            }
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeSymbol(this.Mechanism, buffer);
            AmqpCodec.EncodeBinary(this.InitialResponse, buffer);
            AmqpCodec.EncodeString(this.HostName, buffer);
        }

        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Mechanism = AmqpCodec.DecodeSymbol(buffer);
            }

            if (count-- > 0)
            {
                this.InitialResponse = AmqpCodec.DecodeBinary(buffer);
            }

            if (count-- > 0)
            {
                this.HostName = AmqpCodec.DecodeString(buffer);
            }
        }

        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetSymbolEncodeSize(this.Mechanism);
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.InitialResponse);
            valueSize += AmqpCodec.GetStringEncodeSize(this.HostName);

            return valueSize;
        }
    }
}
