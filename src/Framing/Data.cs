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

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Data() : base(Name, Code)
        {
        }

        internal override int GetValueEncodeSize()
        {
            return BinaryEncoding.GetEncodeSize((ArraySegment<byte>)this.Value);
        }

        internal override void EncodeValue(ByteBuffer buffer)
        {
            BinaryEncoding.Encode((ArraySegment<byte>)this.Value, buffer);
        }

        internal override void DecodeValue(ByteBuffer buffer)
        {
            this.Value = BinaryEncoding.Decode(buffer, 0);
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
