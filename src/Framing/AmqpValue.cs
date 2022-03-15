// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the AMQP value body type of a message.
    /// </summary>
    public sealed class AmqpValue : AmqpDescribed
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public static readonly string Name = "amqp:amqp-value:*";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public static readonly ulong Code = 0x0000000000000077;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public AmqpValue()
            : base(Name, Code)
        {
        }

        internal override int GetValueEncodeSize()
        {
            IAmqpSerializable amqpSerializable = this.Value as IAmqpSerializable;
            if (amqpSerializable != null)
            {
                return amqpSerializable.EncodeSize;
            }
            else
            {
                return base.GetValueEncodeSize();
            }
        }

        internal override void EncodeValue(ByteBuffer buffer)
        {
            IAmqpSerializable amqpSerializable = this.Value as IAmqpSerializable;
            if (amqpSerializable != null)
            {
                amqpSerializable.Encode(buffer);
            }
            else
            {
                base.EncodeValue(buffer);
            }
        }

        internal override void DecodeValue(ByteBuffer buffer)
        {
            this.Value = AmqpCodec.DecodeObject(buffer);
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            return "value()";
        }
    }
}
