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
        public static readonly string Name = "amqp:coordinator:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000030;
        const int Fields = 1;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Coordinator() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the capabilities field.
        /// </summary>
        public Multiple<AmqpSymbol> Capabilities { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        internal override void EnsureRequired()
        {
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeMultiple(this.Capabilities, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Capabilities = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }
        }

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetMultipleEncodeSize(this.Capabilities);

            return valueSize;
        }
    }
}
