// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Encodes and decodes AMQP primitive types. The input/output buffer
    /// is advanced after the read/write operation.
    /// </summary>
    public static class AmqpBitConverter
    {
        /// <summary>
        /// Reads an 8-bit signed number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>An 8-bit signed number.</returns>
        public static sbyte ReadByte(ByteBuffer buffer)
        {
            buffer.ValidateRead(FixedWidth.Byte);
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
            buffer.ValidateRead(FixedWidth.UByte);
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
            short data = BinaryPrimitives.ReadInt16BigEndian(buffer.GetReadSpan(FixedWidth.Short));
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
            ushort data = BinaryPrimitives.ReadUInt16BigEndian(buffer.GetReadSpan(FixedWidth.UShort));
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
            int data = BinaryPrimitives.ReadInt32BigEndian(buffer.GetReadSpan(FixedWidth.Int));
            buffer.Complete(FixedWidth.Int);
            return data;
        }

        /// <summary>
        /// Reads a 32-bit unsigned number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 32-bit unsigned number.</returns>
        public static uint ReadUInt(ByteBuffer buffer)
        {
            uint data = BinaryPrimitives.ReadUInt32BigEndian(buffer.GetReadSpan(FixedWidth.UInt));
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
            return BinaryPrimitives.ReadUInt32BigEndian(buffer.AsSpan<byte>(offset, count));
        }

        internal static ushort ReadUShort(byte[] buffer, int offset, int count)
        {
            return BinaryPrimitives.ReadUInt16BigEndian(buffer.AsSpan<byte>(offset, count));
        }

        internal static uint ReadUInt(ReadOnlySpan<byte> buffer, int offset, int count)
        {
            return BinaryPrimitives.ReadUInt32BigEndian(buffer.Slice(offset, count));
        }

        /// <summary>
        /// Reads a 64-bit signed number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 64-bit signed number.</returns>
        public static long ReadLong(ByteBuffer buffer)
        {
            long data = BinaryPrimitives.ReadInt64BigEndian(buffer.GetReadSpan(FixedWidth.Long));
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
            buffer.ValidateRead(FixedWidth.ULong);
            ulong data = BinaryPrimitives.ReadUInt64BigEndian(buffer.GetReadSpan(FixedWidth.ULong));
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
            return BinaryPrimitives.ReadUInt64BigEndian(buffer.AsSpan(offset, count));
        }

        /// <summary>
        /// Reads a 32-bit floating point number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 32-bit floating point number.</returns>
        public static float ReadFloat(ByteBuffer buffer)
        {
            int data = BinaryPrimitives.ReadInt32BigEndian(buffer.GetReadSpan(FixedWidth.Int));
            buffer.Complete(FixedWidth.Float);
            return Unsafe.As<int, float>(ref data);
        }

        /// <summary>
        /// Reads a 64-bit floating point number from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A 64-bit floating point number.</returns>
        public static double ReadDouble(ByteBuffer buffer)
        {
            long data = BinaryPrimitives.ReadInt64BigEndian(buffer.GetReadSpan(FixedWidth.Long));
            buffer.Complete(FixedWidth.Double);
            return Unsafe.As<long, double>(ref data);
        }

        /// <summary>
        /// Reads a uuid from a buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>A uuid.</returns>
        public static unsafe Guid ReadUuid(ByteBuffer buffer)
        {
            buffer.ValidateRead(FixedWidth.Uuid);
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
            buffer.ValidateRead(count);
            Buffer.BlockCopy(buffer.Buffer, buffer.Offset, data, offset, count);
            buffer.Complete(count);
        }

        internal static void ReadBytes(ByteBuffer buffer, Span<byte> data, int offset, int count)
        {
            buffer.GetReadSpan(count).Slice(offset, count).CopyTo(data);
            buffer.Complete(count);
        }

        /// <summary>
        /// Writes an 8-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 8-bit signed number.</param>
        public static void WriteByte(ByteBuffer buffer, sbyte data)
        {
            buffer.ValidateWrite(FixedWidth.Byte);
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
            buffer.ValidateWrite(FixedWidth.UByte);
            buffer.Buffer[buffer.WritePos] = data;
            buffer.Append(FixedWidth.UByte);
        }

        /// <summary>
        /// Writes a 16-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 16-bit signed number.</param>
        public static void WriteShort(ByteBuffer buffer, short data)
        {
            BinaryPrimitives.WriteInt16BigEndian(buffer.GetWriteSpan(FixedWidth.Short), data);
            buffer.Append(FixedWidth.Short);
        }

        /// <summary>
        /// Writes a 16-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 16-bit unsigned number.</param>
        public static void WriteUShort(ByteBuffer buffer, ushort data)
        {
            BinaryPrimitives.WriteUInt16BigEndian(buffer.GetWriteSpan(FixedWidth.UShort), data);
            buffer.Append(FixedWidth.UShort);
        }

        /// <summary>
        /// Writes a 32-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 32-bit signed number.</param>
        public static void WriteInt(ByteBuffer buffer, int data)
        {
            BinaryPrimitives.WriteInt32BigEndian(buffer.GetWriteSpan(FixedWidth.Int), data);
            buffer.Append(FixedWidth.Int);
        }

        /// <summary>
        /// Writes a 32-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 32-bit unsigned number.</param>
        public static void WriteUInt(ByteBuffer buffer, uint data)
        {
            BinaryPrimitives.WriteUInt32BigEndian(buffer.GetWriteSpan(FixedWidth.UInt), data);
            buffer.Append(FixedWidth.UInt);
        }

        /// <summary>
        /// Writes a 16-bit unsigned number to a byte array.
        /// </summary>
        /// <param name="buffer">The byte array to write.</param>
        /// <param name="offset">The offset in the array to write.</param>
        /// <param name="data">The 16-bit unsigned number.</param>
        public static void WriteUShort(byte[] buffer, int offset, ushort data)
        {
            BinaryPrimitives.WriteUInt16BigEndian(buffer.AsSpan(offset, FixedWidth.UShort), data);
        }

        /// <summary>
        /// Writes a 32-bit unsigned number to a byte array.
        /// </summary>
        /// <param name="buffer">The byte array to write.</param>
        /// <param name="offset">The offset in the array to write.</param>
        /// <param name="data">The 32-bit unsigned number.</param>
        public static void WriteUInt(byte[] buffer, int offset, uint data)
        {
            BinaryPrimitives.WriteUInt32BigEndian(buffer.AsSpan(offset, FixedWidth.UInt), data);
        }

        /// <summary>
        /// Writes a 64-bit unsigned number to a byte array.
        /// </summary>
        /// <param name="buffer">The byte array to write.</param>
        /// <param name="offset">The offset in the array to write.</param>
        /// <param name="data">The 64-bit unsigned number.</param>
        public static void WriteULong(byte[] buffer, int offset, ulong data)
        {
            BinaryPrimitives.WriteUInt64BigEndian(buffer.AsSpan(offset, FixedWidth.ULong), data);
        }

        /// <summary>
        /// Writes a 64-bit signed number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 64-bit signed number.</param>
        public static void WriteLong(ByteBuffer buffer, long data)
        {
            BinaryPrimitives.WriteInt64BigEndian(buffer.GetWriteSpan(FixedWidth.Long), data);
            buffer.Append(FixedWidth.Long);
        }

        /// <summary>
        /// Writes a 64-bit unsigned number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 64-bit unsigned number.</param>
        public static void WriteULong(ByteBuffer buffer, ulong data)
        {
            BinaryPrimitives.WriteUInt64BigEndian(buffer.GetWriteSpan(FixedWidth.ULong), data);
            buffer.Append(FixedWidth.ULong);
        }

        /// <summary>
        /// Writes a 32-bit floating point number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 32-bit floating point number.</param>
        public static void WriteFloat(ByteBuffer buffer, float data)
        {
            BinaryPrimitives.WriteInt32BigEndian(buffer.GetWriteSpan(FixedWidth.Float), Unsafe.As<float, int>(ref data));
            buffer.Append(FixedWidth.Float);
        }

        /// <summary>
        /// Writes a 64-bit floating point number to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The 64-bit floating point number.</param>
        public static void WriteDouble(ByteBuffer buffer, double data)
        {
            BinaryPrimitives.WriteInt64BigEndian(buffer.GetWriteSpan(FixedWidth.Double), Unsafe.As<double, long>(ref data));
            buffer.Append(FixedWidth.Double);
        }

        /// <summary>
        /// Writes a uuid to a buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        /// <param name="data">The uuid.</param>
        public static unsafe void WriteUuid(ByteBuffer buffer, Guid data)
        {
            buffer.ValidateWrite(FixedWidth.Uuid);
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
            buffer.ValidateWrite(count);
            Buffer.BlockCopy(data, offset, buffer.Buffer, buffer.WritePos, count);
            buffer.Append(count);
        }

        internal static void WriteBytes(ByteBuffer buffer, ReadOnlySpan<byte> data, int offset, int count)
        {
            data.CopyTo(buffer.GetWriteSpan(count));
            buffer.Append(count);
        }

        internal static void Write(ByteBuffer buffer, byte b1, byte b2)
        {
            buffer.ValidateWrite(2);
            buffer.Buffer[buffer.WritePos] = b1;
            buffer.Buffer[buffer.WritePos + 1] = b2;
            buffer.Append(2);
        }

        internal static void Write(ByteBuffer buffer, byte b1, uint b2)
        {
            buffer.ValidateWrite(5);
            buffer.Buffer[buffer.WritePos] = b1;
            WriteUInt(buffer.Buffer, buffer.WritePos + 1, b2);
            buffer.Append(5);
        }
    }
}
