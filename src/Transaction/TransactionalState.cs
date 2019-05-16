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
        public static readonly string Name = "amqp:transactional-state:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000034;
        const int Fields = 2;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public TransactionalState() : base(Name, Code) { }

        /// <summary>
        /// Gets or sets the txn-id field.
        /// </summary>
        public ArraySegment<byte> TxnId { get; set; }

        /// <summary>
        /// Gets or sets the outcome field.
        /// </summary>
        public Outcome Outcome { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

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

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.TxnId, buffer);
            AmqpCodec.EncodeSerializable(this.Outcome, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
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

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetBinaryEncodeSize(this.TxnId);
            valueSize += AmqpCodec.GetSerializableEncodeSize(this.Outcome);

            return valueSize;
        }
    }
}
