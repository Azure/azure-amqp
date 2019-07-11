// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    /// <summary>
    /// Encodes and decodes AMQP primitive types. The input/output buffer
    /// is advanced after the read/write operation.
    /// </summary>
    public static unsafe class AmqpBitConverter
    {
        /// <summary>
        /// Reads an 8-bit signed number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>An 8-bit signed number.</returns>
        public static sbyte ReadByte(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Byte);
            sbyte data = (sbyte)buffer.Buffer[buffer.Offset];
            buffer.Complete(FixedWidth.Byte);
            return data;
        }

        /// <summary>
        /// Reads an 8-bit unsigned number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>An 8-bit unsigned number.</returns>
        public static byte ReadUByte(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UByte);
            byte data = buffer.Buffer[buffer.Offset];
            buffer.Complete(FixedWidth.UByte);
            return data;
        }

        /// <summary>
        /// Reads a 16-bit signed number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 16-bit signed number.</returns>
        public static short ReadShort(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Short);
            short data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[1];
                d[1] = p[0];
            }

            buffer.Complete(FixedWidth.Short);
            return data;
        }

        /// <summary>
        /// Reads a 16-bit unsigned number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 16-bit unsigned number.</returns>
        public static ushort ReadUShort(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UShort);
            ushort data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[1];
                d[1] = p[0];
            }

            buffer.Complete(FixedWidth.UShort);
            return data;
        }

        /// <summary>
        /// Reads a 32-bit signed number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 32-bit signed number.</returns>
        public static int ReadInt(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Int);
            int data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }

            buffer.Complete(FixedWidth.Int);
            return data;
        }

        internal static uint PeekUInt(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UInt);
            uint data = ReadUInt(buffer.Buffer, buffer.Offset, buffer.Length);
            return data;
        }

        /// <summary>
        /// Reads a 32-bit unsigned number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 32-bit unsigned number.</returns>
        public static uint ReadUInt(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UInt);
            uint data = ReadUInt(buffer.Buffer, buffer.Offset, buffer.Length);
            buffer.Complete(FixedWidth.UInt);
            return data;
        }

        /// <summary>
        /// Reads a 32-bit unsigned number from a byte array.
        /// </summary>
        /// <param name="buffer">The input byte array</param>
        /// <param name="offset">The offset to read.</param>
        /// <param name="count">bytes available from offset in the array.</param>
        /// <returns>A 32-bit unsigned number.</returns>
        public static uint ReadUInt(byte[] buffer, int offset, int count)
        {
            Validate(count, FixedWidth.UInt);
            uint data;
            fixed (byte* p = &buffer[offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }

            return data;
        }

        /// <summary>
        /// Reads a 64-bit signed number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 64-bit signed number.</returns>
        public static long ReadLong(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Long);
            long data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[7];
                d[1] = p[6];
                d[2] = p[5];
                d[3] = p[4];
                d[4] = p[3];
                d[5] = p[2];
                d[6] = p[1];
                d[7] = p[0];
            }

            buffer.Complete(FixedWidth.Long);
            return data;
        }

        /// <summary>
        /// Reads a 64-bit unsigned number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 64-bit unsigned number.</returns>
        public static ulong ReadULong(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.ULong);
            ulong data = ReadULong(buffer.Buffer, buffer.Offset, buffer.Length);
            buffer.Complete(FixedWidth.ULong);
            return data;
        }


        /// <summary>
        /// Reads a 64-bit unsigned number from a byte array.
        /// </summary>
        /// <param name="buffer">The input byte array</param>
        /// <param name="offset">The offset to read.</param>
        /// <param name="count">bytes available from offset in the array.</param>
        /// <returns>A 64-bit unsigned number.</returns>
        public static ulong ReadULong(byte[] buffer, int offset, int count)
        {
            Validate(count, FixedWidth.ULong);
            ulong data;
            fixed (byte* p = &buffer[offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[7];
                d[1] = p[6];
                d[2] = p[5];
                d[3] = p[4];
                d[4] = p[3];
                d[5] = p[2];
                d[6] = p[1];
                d[7] = p[0];
            }

            return data;
        }

        /// <summary>
        /// Reads a 32-bit floating point number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 32-bit floating point number.</returns>
        public static float ReadFloat(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Float);
            float data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }

            buffer.Complete(FixedWidth.Float);
            return data;
        }

        /// <summary>
        /// Reads a 64-bit floating point number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 64-bit floating point number.</returns>
        public static double ReadDouble(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Double);
            double data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[7];
                d[1] = p[6];
                d[2] = p[5];
                d[3] = p[4];
                d[4] = p[3];
                d[5] = p[2];
                d[6] = p[1];
                d[7] = p[0];
            }

            buffer.Complete(FixedWidth.Double);
            return data;
        }

        /// <summary>
        /// Reads a uuid from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A uuid.</returns>
        public static Guid ReadUuid(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Uuid);
            Guid data;
            fixed (byte* p = &buffer.Buffer[buffer.Offset])
            {
                byte* d = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];

                d[4] = p[5];
                d[5] = p[4];

                d[6] = p[7];
                d[7] = p[6];

                *((ulong*)&d[8]) = *((ulong*)&p[8]);
            }

            buffer.Complete(FixedWidth.Uuid);
            return data;
        }

        /// <summary>
        /// Reads a number of bytes from a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to read.</param>
        /// <param name="data">The destination byte array.</param>
        /// <param name="offset">The offset of the destination byte array.</param>
        /// <param name="count">The number of bytes to read.</param>
        public static void ReadBytes(ByteBuffer buffer, byte[] data, int offset, int count)
        {
            buffer.Validate(false, count);
            Buffer.BlockCopy(buffer.Buffer, buffer.Offset, data, offset, count);
            buffer.Complete(count);
        }

        /// <summary>
        /// Writes an 8-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 8-bit signed number.</param>
        public static void WriteByte(ByteBuffer buffer, sbyte data)
        {
            buffer.Validate(true, FixedWidth.Byte);
            buffer.Buffer[buffer.WritePos] = (byte)data;
            buffer.Append(FixedWidth.Byte);
        }

        /// <summary>
        /// Writes an 8-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 8-bit unsigned number.</param>
        public static void WriteUByte(ByteBuffer buffer, byte data)
        {
            buffer.Validate(true, FixedWidth.UByte);
            buffer.Buffer[buffer.WritePos] = data;
            buffer.Append(FixedWidth.UByte);
        }

        internal static void WriteUByte(byte[] buffer, int offset, byte data)
        {
            Validate(buffer.Length - offset, FixedWidth.UByte);
            buffer[offset] = data;
        }

        /// <summary>
        /// Writes a 16-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 16-bit signed number.</param>
        public static void WriteShort(ByteBuffer buffer, short data)
        {
            buffer.Validate(true, FixedWidth.Short);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[1];
                d[1] = p[0];
            }

            buffer.Append(FixedWidth.Short);
        }

        /// <summary>
        /// Writes a 16-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 16-bit unsigned number.</param>
        public static void WriteUShort(ByteBuffer buffer, ushort data)
        {
            buffer.Validate(true, FixedWidth.UShort);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[1];
                d[1] = p[0];
            }

            buffer.Append(FixedWidth.UShort);
        }

        /// <summary>
        /// Writes a 32-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 32-bit signed number.</param>
        public static void WriteInt(ByteBuffer buffer, int data)
        {
            buffer.Validate(true, FixedWidth.Int);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }

            buffer.Append(FixedWidth.Int);
        }

        /// <summary>
        /// Writes a 32-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 32-bit unsigned number.</param>
        public static void WriteUInt(ByteBuffer buffer, uint data)
        {
            buffer.Validate(true, FixedWidth.UInt);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }

            buffer.Append(FixedWidth.UInt);
        }

        /// <summary>
        /// Writes a 32-bit unsigned number to a byte array.
        /// </summary>
        /// <param name="buffer">The byte array to write.</param>
        /// <param name="offset">The offset in the array to write.</param>
        /// <param name="data">The 32-bit unsigned number.</param>
        public static void WriteUInt(byte[] buffer, int offset, uint data)
        {
            Validate(buffer.Length - offset, FixedWidth.UInt);
            fixed (byte* d = &buffer[offset])
            {
                byte* p = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }
        }

        /// <summary>
        /// Writes a 64-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 64-bit signed number.</param>
        public static void WriteLong(ByteBuffer buffer, long data)
        {
            buffer.Validate(true, FixedWidth.Long);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[7];
                d[1] = p[6];
                d[2] = p[5];
                d[3] = p[4];
                d[4] = p[3];
                d[5] = p[2];
                d[6] = p[1];
                d[7] = p[0];
            }

            buffer.Append(FixedWidth.Long);
        }

        /// <summary>
        /// Writes a 64-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 64-bit unsigned number.</param>
        public static void WriteULong(ByteBuffer buffer, ulong data)
        {
            buffer.Validate(true, FixedWidth.ULong);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[7];
                d[1] = p[6];
                d[2] = p[5];
                d[3] = p[4];
                d[4] = p[3];
                d[5] = p[2];
                d[6] = p[1];
                d[7] = p[0];
            }

            buffer.Append(FixedWidth.ULong);
        }

        /// <summary>
        /// Writes a 32-bit floating point number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 32-bit floating point number.</param>
        public static void WriteFloat(ByteBuffer buffer, float data)
        {
            buffer.Validate(true, FixedWidth.Float);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];
            }

            buffer.Append(FixedWidth.Float);
        }

        /// <summary>
        /// Writes a 64-bit floating point number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 64-bit floating point number.</param>
        public static void WriteDouble(ByteBuffer buffer, double data)
        {
            buffer.Validate(true, FixedWidth.Double);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[7];
                d[1] = p[6];
                d[2] = p[5];
                d[3] = p[4];
                d[4] = p[3];
                d[5] = p[2];
                d[6] = p[1];
                d[7] = p[0];
            }

            buffer.Append(FixedWidth.Double);
        }

        /// <summary>
        /// Writes a uuid to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The uuid.</param>
        public static void WriteUuid(ByteBuffer buffer, Guid data)
        {
            buffer.Validate(true, FixedWidth.Uuid);
            fixed (byte* d = &buffer.Buffer[buffer.WritePos])
            {
                byte* p = (byte*)&data;
                d[0] = p[3];
                d[1] = p[2];
                d[2] = p[1];
                d[3] = p[0];

                d[4] = p[5];
                d[5] = p[4];

                d[6] = p[7];
                d[7] = p[6];

                *((ulong*)&d[8]) = *((ulong*)&p[8]);
            }

            buffer.Append(FixedWidth.Uuid);
        }

        /// <summary>
        /// Writes a number of bytes to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The source byte array.</param>
        /// <param name="offset">The offset of the source byte array.</param>
        /// <param name="count">The number of bytes.</param>
        public static void WriteBytes(ByteBuffer buffer, byte[] data, int offset, int count)
        {
            buffer.Validate(true, count);
            Buffer.BlockCopy(data, offset, buffer.Buffer, buffer.WritePos, count);
            buffer.Append(count);
        }

        static void Validate(int bufferSize, int dataSize)
        {
            if (bufferSize < dataSize)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, AmqpResources.GetString(AmqpResources.AmqpInsufficientBufferSize, dataSize, bufferSize));
            }
        }
    }
}
