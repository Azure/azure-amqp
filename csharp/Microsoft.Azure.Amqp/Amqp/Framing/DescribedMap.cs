// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class DescribedMap : AmqpDescribed
    {
        AmqpMap innerMap;

        public DescribedMap(AmqpSymbol name, ulong code)
            : base(name, code)
        {
            this.innerMap = new AmqpMap();
        }

        protected AmqpMap InnerMap
        {
            get { return this.innerMap; }
        }

        public override int GetValueEncodeSize()
        {
            return MapEncoding.GetEncodeSize(this.innerMap);
        }

        public override void EncodeValue(ByteBuffer buffer)
        {
            MapEncoding.Encode(this.innerMap, buffer);
        }

        public override void DecodeValue(ByteBuffer buffer)
        {
            this.innerMap = MapEncoding.Decode(buffer, 0);
        }

        public void DecodeValue(ByteBuffer buffer, int size, int count)
        {
            MapEncoding.ReadMapValue(buffer, this.innerMap, size, count);
        }
    }
}
