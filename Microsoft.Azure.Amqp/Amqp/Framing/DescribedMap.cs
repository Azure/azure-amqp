// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class DescribedMap : AmqpDescribed
    {
        public DescribedMap(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }

        internal abstract AmqpMap InnerMap { get; }

        public override int GetValueEncodeSize()
        {
            return MapEncoding.GetEncodeSize(this.InnerMap);
        }

        public override void EncodeValue(ByteBuffer buffer)
        {
            MapEncoding.Encode(this.InnerMap, buffer);
        }

        public override void DecodeValue(ByteBuffer buffer)
        {
            var formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode != FormatCode.Null)
            {
                AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Map8, FormatCode.Map32, out int size, out int count);
                MapEncoding.ReadMapValue(buffer, this.InnerMap, size, count);
            }
        }

        public void DecodeValue(ByteBuffer buffer, int size, int count)
        {
            MapEncoding.ReadMapValue(buffer, this.InnerMap, size, count);
        }
    }
}
