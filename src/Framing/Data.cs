// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Data : AmqpDescribed
    {
        public static readonly string Name = "amqp:data:binary";
        public static readonly ulong Code = 0x0000000000000075;

        public Data() : base(Name, Code)
        {
        }

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
        
        public override int GetValueEncodeSize()
        {
            return BinaryEncoding.GetEncodeSize((ArraySegment<byte>)this.Value);
        }

        public override void EncodeValue(ByteBuffer buffer)
        {
            BinaryEncoding.Encode((ArraySegment<byte>)this.Value, buffer);
        }

        public override void DecodeValue(ByteBuffer buffer)
        {
            this.Value = BinaryEncoding.Decode(buffer, 0);
        }

        public override string ToString()
        {
            return "data()";
        }
    }
}
