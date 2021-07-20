// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Diagnostics;

    struct SizeTracker
    {
        ByteBuffer buffer;
        int trackedLength;

        public int Length
        {
            get { return buffer.Length - this.trackedLength; }
        }

        public static SizeTracker Track(ByteBuffer buffer)
        {
            return new SizeTracker() { buffer = buffer, trackedLength = buffer.Length };
        }

        // Write the tracked size into the tracked position. Size is inclusive.
        public void CommitInclusive(int extra)
        {
            int absPos = this.buffer.Offset + this.trackedLength;
            AmqpBitConverter.WriteUInt(buffer.Buffer, absPos, (uint)(this.Length + extra));
        }

        // Write the tracked size into the tracked position by an offset. Size is exclusive.
        public void CommitExclusive(int offset)
        {
            Debug.Assert(offset < this.Length);
            int absPos = this.buffer.Offset + this.trackedLength + offset;
            int size = buffer.WritePos - absPos - FixedWidth.Int;
            AmqpBitConverter.WriteUInt(buffer.Buffer, absPos, (uint)size);
        }

        // Compact range (from pos) into 3 bytes and left shift remaining bytes.
        public void Compact(byte b1, byte b2, byte b3, int range)
        {
            Debug.Assert(range > 3);
            int absPos = this.buffer.Offset + this.trackedLength;
            int bytes = this.Length - range;
            buffer.Buffer[absPos] = b1;
            buffer.Buffer[absPos + 1] = b2;
            buffer.Buffer[absPos + 2] = b3;
            Buffer.BlockCopy(buffer.Buffer, absPos + range, buffer.Buffer, absPos + 3, bytes);
            buffer.AdjustPosition(buffer.Offset, buffer.Length - (range - 3));
        }
    }
}
