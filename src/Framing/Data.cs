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
        /// Gets the encoded prefix bytes for a data section with the given value length.
        /// </summary>
        /// <param name="valueLength">The length of the binary value.</param>
        /// <returns>An array segment containing the encoded prefix.</returns>
        [Obsolete("Obsolete")]
        public static ArraySegment<byte> GetEncodedPrefix(int valueLength)
        {
            byte[] buffer = new byte[8] { FormatCode.Described, FormatCode.SmallULong, (byte)Data.Code, 0x00, 0x00, 0x00, 0x00, 0x00 };
            int count;
            if (valueLength <= byte.MaxValue)
            {
                buffer[3] = FormatCode.Binary8;
                buffer[4] = (byte)valueLength;
                count = 5;
            }
            else
            {
                buffer[3] = FormatCode.Binary32;
                AmqpBitConverter.WriteUInt(buffer, 4, (uint)valueLength);
                count = 8;
            }

            return new ArraySegment<byte>(buffer, 0, count);
        }

        /// <summary>
        /// Gets or sets the value. If this property is used, the <see cref="DescribedType.Value"/>
        /// base property must not be used.
        /// </summary>
        internal ArraySegment<byte> Segment
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

        /// <inheritdoc/>
        public override int GetValueEncodeSize()
        {
            return AmqpCodec.GetBinaryEncodeSize(this.Segment);
        }

        /// <inheritdoc/>
        public override void EncodeValue(ByteBuffer buffer)
        {
            AmqpCodec.EncodeBinary(this.Segment, buffer);
        }

        /// <inheritdoc/>
        public override void DecodeValue(ByteBuffer buffer)
        {
            this.segment = AmqpCodec.DecodeBinary(buffer);
            // Have to set this for back compat
            this.Value = this.segment;
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
