// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines the declare performative.
    /// </summary>
    public sealed class Declare : Performative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:declare:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000031;
        const int Fields = 1;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Declare() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the global-id field.
        /// </summary>
        public object GlobalId { get; set; }

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
            return "declare()";
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeObject(this.GlobalId, buffer);
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
                this.GlobalId = AmqpEncoding.DecodeObject(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetObjectEncodeSize(this.GlobalId);

            return valueSize;
        }
    }
}
