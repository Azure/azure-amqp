// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    public sealed class SaslResponse : Performative
    {
        public static readonly string Name = "amqp:sasl-response:list";
        public static readonly ulong Code = 0x0000000000000043;
        const int Fields = 1;

        public SaslResponse() : base(Name, Code) { }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        public ArraySegment<byte> Response { get; set; }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("sasl-response(");
            int count = 0;
            this.AddFieldToString(this.Response.Array != null, sb, "response", this.Response, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        protected override void EnsureRequired()
        {
            if (this.Response.Array == null)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-response:response");
            }
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.Response, buffer);
        }

        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Response = AmqpCodec.DecodeBinary(buffer);
            }
        }

        protected override int OnValueSize()
        {
            int valueSize = 0;
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.Response);
            return valueSize;
        }
    }
}
