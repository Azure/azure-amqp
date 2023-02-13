// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines the coordinator target.
    /// </summary>
    public sealed class Coordinator : DescribedList
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:coordinator:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000030;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Coordinator() : base(Name, Code, 1)
        {
        }

        /// <summary>
        /// Gets or sets the capabilities field.
        /// </summary>
        public Multiple<AmqpSymbol> Capabilities { get; set; }

        internal override void EnsureRequired()
        {
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeMultiple(this.Capabilities, buffer);
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
                this.Capabilities = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetMultipleEncodeSize(this.Capabilities);

            return valueSize;
        }
    }
}
