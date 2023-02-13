// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Security mechanism response.
    /// </summary>
    public sealed class SaslResponse : Performative
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public const string Name = "amqp:sasl-response:list";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public const ulong Code = 0x0000000000000043;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslResponse() : base(Name, Code, 1) { }

        /// <summary>
        /// Gets or sets the security response data.
        /// </summary>
        public ArraySegment<byte> Response { get; set; }

        /// <summary>
        /// Gets a string representing the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("sasl-response(");
            int count = 0;
            this.AddFieldToString(this.Response.Array != null, sb, "response", this.Response, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (this.Response.Array == null)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-response:response");
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.Response, buffer);
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
                this.Response = AmqpCodec.DecodeBinary(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.Response);
            return valueSize;
        }
    }
}
