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
        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="name">The descriptor name.</param>
        /// <param name="code">The descriptor code.</param>
        public DescribedMap(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }

        /// <summary>
        /// Gets the map that stores the key-value items.
        /// </summary>
        internal abstract AmqpMap InnerMap { get; }

        /// <inheritdoc/>
        public override int GetValueEncodeSize()
        {
            return MapEncoding.GetEncodeSize(this.InnerMap);
        }

        /// <inheritdoc/>
        public override void EncodeValue(ByteBuffer buffer)
        {
            MapEncoding.Encode(this.InnerMap, buffer);
        }

        /// <inheritdoc/>
        public override void DecodeValue(ByteBuffer buffer)
        {
            var formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode != FormatCode.Null)
            {
                AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Map8, FormatCode.Map32, out int size, out int count);
                MapEncoding.ReadMapValue(buffer, this.InnerMap, size, count);
            }
        }

        /// <summary>Decodes the described map value from the buffer.</summary>
        public void DecodeValue(ByteBuffer buffer, int size, int count)
        {
            MapEncoding.ReadMapValue(buffer, this.InnerMap, size, count);
        }
    }
}
