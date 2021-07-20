// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines a described type whose value is a map.
    /// </summary>
    public abstract class DescribedMap : AmqpDescribed
    {
        AmqpMap innerMap;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="name">The descriptor name.</param>
        /// <param name="code">The descriptor code.</param>
        public DescribedMap(AmqpSymbol name, ulong code)
            : base(name, code)
        {
            this.innerMap = new AmqpMap();
        }

        /// <summary>
        /// Gets the map that stores the key-value items.
        /// </summary>
        protected AmqpMap InnerMap
        {
            get { return this.innerMap; }
        }

        internal override int GetValueEncodeSize()
        {
            return AmqpCodec.GetMapEncodeSize(this.innerMap);
        }

        internal override void EncodeValue(ByteBuffer buffer)
        {
            AmqpCodec.EncodeMap(this.innerMap, buffer);
        }

        internal override void DecodeValue(ByteBuffer buffer)
        {
            this.innerMap = AmqpCodec.DecodeMap(buffer);
        }

        internal void DecodeValue(ByteBuffer buffer, int size, int count)
        {
            MapEncoding.ReadMapValue(buffer, this.innerMap, size, count);
        }
    }
}
