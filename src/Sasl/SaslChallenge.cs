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
        public static readonly string Name = "amqp:sasl-challenge:list";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public static readonly ulong Code = 0x0000000000000042;
        const int Fields = 1;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslChallenge() : base(Name, Code) { }

        internal override int FieldCount
        {
            get { return Fields; }
        }

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

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.Challenge, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Challenge = AmqpCodec.DecodeBinary(buffer);
            }
        }

        internal override int OnValueSize()
        {
            int valueSize = 0;
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.Challenge);
            return valueSize;
        }
    }
}
