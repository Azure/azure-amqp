// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines the discharge performative.
    /// </summary>
    public sealed class Discharge : Performative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:discharge:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000032;
        const int Fields = 2;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Discharge() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the txn-id field.
        /// </summary>
        public ArraySegment<byte> TxnId { get; set; }

        /// <summary>
        /// Gets or sets the fail field.
        /// </summary>
        public bool? Fail { get; set; }

        /// <summary>
        /// Gets the number of fields in the list.
        /// </summary>
        protected override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("discharge(");
            int count = 0;
            this.AddFieldToString(this.TxnId.Array != null, sb, "txn-id", this.TxnId, ref count);
            this.AddFieldToString(this.Fail != null, sb, "fail", this.Fail, ref count);
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
            AmqpCodec.EncodeBoolean(this.Fail, buffer);
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
                this.Fail = AmqpCodec.DecodeBoolean(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize = AmqpCodec.GetBinaryEncodeSize(this.TxnId);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Fail);

            return valueSize;
        }
    }
}
