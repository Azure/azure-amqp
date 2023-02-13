// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Security mechanism challenge.
    /// </summary>
    public sealed class SaslChallenge : Performative
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public const string Name = "amqp:sasl-challenge:list";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public const ulong Code = 0x0000000000000042;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslChallenge() : base(Name, Code, 1) { }

        /// <summary>
        /// Gets or sets the security challenge data.
        /// </summary>
        public ArraySegment<byte> Challenge { get; set; }

        /// <summary>
        /// Gets a string representing the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("sasl-challenge(");
            int count = 0;
            this.AddFieldToString(this.Challenge.Array != null, sb, "challenge", this.Challenge, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (this.Challenge.Array == null)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-challenge:challenge");
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.Challenge, buffer);
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
                this.Challenge = AmqpCodec.DecodeBinary(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.Challenge);
            return valueSize;
        }
    }
}
