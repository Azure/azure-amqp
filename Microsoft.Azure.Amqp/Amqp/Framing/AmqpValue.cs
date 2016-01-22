// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class AmqpValue : AmqpDescribed
    {
        public static readonly string Name = "amqp:amqp-value:*";
        public static readonly ulong Code = 0x0000000000000077;

        public AmqpValue()
            : base(Name, Code)
        {
        }

        public override int GetValueEncodeSize()
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

        public override void EncodeValue(ByteBuffer buffer)
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

        public override void DecodeValue(ByteBuffer buffer)
        {
            this.Value = AmqpCodec.DecodeObject(buffer);
        }

        public override string ToString()
        {
            return "value()";
        }
    }
}
