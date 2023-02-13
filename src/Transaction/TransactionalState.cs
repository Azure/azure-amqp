// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines the transaction state.
    /// </summary>
    public sealed class TransactionalState : DeliveryState
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:transactional-state:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000034;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public TransactionalState() : base(Name, Code, 2) { }

        /// <summary>
        /// Gets or sets the txn-id field.
        /// </summary>
        public ArraySegment<byte> TxnId { get; set; }

        /// <summary>
        /// Gets or sets the outcome field.
        /// </summary>
        public Outcome Outcome { get; set; }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("txn-state(");
            int count = 0;
            this.AddFieldToString(this.TxnId.Array != null, sb, "txn-id", this.TxnId, ref count);
            this.AddFieldToString(this.Outcome != null, sb, "outcome", this.Outcome, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.TxnId, buffer);
            AmqpCodec.EncodeSerializable(this.Outcome, buffer);
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
                this.TxnId = AmqpCodec.DecodeBinary(buffer);
            }

            if (count-- > 0)
            {
                this.Outcome = (Outcome)AmqpCodec.DecodeAmqpDescribed(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetBinaryEncodeSize(this.TxnId);
            valueSize += AmqpCodec.GetSerializableEncodeSize(this.Outcome);

            return valueSize;
        }
    }
}
