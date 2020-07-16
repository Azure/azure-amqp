// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Indicates the outcome of the sasl dialog.
    /// </summary>
    public sealed class SaslOutcome : Performative
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public static readonly string Name = "amqp:sasl-outcome:list";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public static readonly ulong Code = 0x0000000000000044;
        const int Fields = 2;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslOutcome() : base(Name, Code) { }

        /// <summary>
        /// Gets the number of fields in the list.
        /// </summary>
        protected override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Gets or sets the code that indicates the outcome of the sasl dialog.
        /// </summary>
        public SaslCode? OutcomeCode { get; set; }

        /// <summary>
        /// Gets or sets the additional data as specified in RFC-4422.
        /// </summary>
        public ArraySegment<byte> AdditionalData { get; set; }

        /// <summary>
        /// Gets a string representing the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("sasl-outcome(");
            int count = 0;
            this.AddFieldToString(this.OutcomeCode != null, sb, "code", this.OutcomeCode, ref count);
            this.AddFieldToString(this.AdditionalData.Array != null, sb, "additional-data", this.AdditionalData, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (this.OutcomeCode == null)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-outcome:code");
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUByte((byte?)this.OutcomeCode, buffer);
            AmqpCodec.EncodeBinary(this.AdditionalData, buffer);
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
                this.OutcomeCode = (SaslCode?)AmqpCodec.DecodeUByte(buffer);
            }

            if (count-- > 0)
            {
                this.AdditionalData = AmqpCodec.DecodeBinary(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;
            valueSize += AmqpCodec.GetUByteEncodeSize((byte?)this.OutcomeCode);
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.AdditionalData);
            return valueSize;
        }
    }
}
