// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the data message section.
    /// </summary>
    public sealed class Data : AmqpDescribed
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:data:binary";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000075;

        ArraySegment<byte> segment;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Data() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the value. If this property is used, the <see cref="DescribedType.Value"/>
        /// base property must not be used.
        /// </summary>
        public ArraySegment<byte> Segment
        {
            get
            {
                if (this.segment.Array != null)
                {
                    return this.segment;
                }

                if (this.Value != null)
                {
                    return (ArraySegment<byte>)this.Value;
                }

                return default;
            }
            set
            {
                this.segment = value;
            }
        }

        internal override int GetValueEncodeSize()
        {
            return AmqpCodec.GetBinaryEncodeSize(this.Segment);
        }

        internal override void EncodeValue(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.Segment, buffer);
        }

        internal override void DecodeValue(ByteBuffer buffer)
        {
            this.Segment = AmqpCodec.DecodeBinary(buffer);
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return "data()";
        }
    }
}
