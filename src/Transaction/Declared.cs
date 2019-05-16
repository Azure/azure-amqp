// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines the declared outcome.
    /// </summary>
    public sealed class Declared : Outcome
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:declared:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000033;
        const int Fields = 1;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Declared() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the txn-id field.
        /// </summary>
        public ArraySegment<byte> TxnId { get; set; }

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
            StringBuilder sb = new StringBuilder("declared(");
            int count = 0;
            this.AddFieldToString(this.TxnId.Array != null, sb, "txn-id", this.TxnId, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.TxnId, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.TxnId = AmqpCodec.DecodeBinary(buffer);
            }
        }

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetBinaryEncodeSize(this.TxnId);

            return valueSize;
        }
    }
}
