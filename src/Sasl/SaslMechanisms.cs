// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Advertise available sasl mechanisms.
    /// </summary>
    public sealed class SaslMechanisms : Performative
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public const string Name = "amqp:sasl-mechanisms:list";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public const ulong Code = 0x0000000000000040;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslMechanisms() : base(Name, Code, 1) { }

        /// <summary>
        /// Gets or sets the supported sasl mechanisms.
        /// </summary>
        public Multiple<AmqpSymbol> SaslServerMechanisms { get; set; }

        /// <summary>
        /// Gets a string representing the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("sasl-mechanisms(");
            int count = 0;
            this.AddFieldToString(this.SaslServerMechanisms != null, sb, "sasl-server-mechanisms", this.SaslServerMechanisms, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (this.SaslServerMechanisms == null)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-mechanisms:sasl-server-mechanisms");
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeMultiple(this.SaslServerMechanisms, buffer);
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
                this.SaslServerMechanisms = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.SaslServerMechanisms);
            return valueSize;
        }
    }
}
