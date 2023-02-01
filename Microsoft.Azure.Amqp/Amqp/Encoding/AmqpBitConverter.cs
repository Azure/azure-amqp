// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    public static unsafe class AmqpBitConverter
    {
        public static sbyte ReadByte(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.Byte);
            sbyte data = (sbyte)buffer.Buffer[buffer.Offset];
            buffer.Complete(FixedWidth.Byte);
            return data;
        }

        public static byte ReadUByte(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UByte);
            byte data = buffer.Buffer[buffer.Offset];
            buffer.Complete(FixedWidth.UByte);
            return data;
        }

        public static short ReadShort(ByteBuffer buffer)
        {
            return (short)ReadUShort(buffer);
        }

        public static ushort ReadUShort(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UShort);
            ushort data = (ushort)((buffer.Buffer[buffer.Offset] << 8) | buffer.Buffer[buffer.Offset + 1]);
            buffer.Complete(FixedWidth.UShort);
            return data;
        }

        public static int ReadInt(ByteBuffer buffer)
        {
            return (int)ReadUInt(buffer);
        }

        public static uint PeekUInt(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UInt);
            uint data = ReadUInt(buffer.Buffer, buffer.Offset, buffer.Length);
            return data;
        }

        public static uint ReadUInt(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.UInt);
            uint data = ReadUInt(buffer.Buffer, buffer.Offset, buffer.Length);
            buffer.Complete(FixedWidth.UInt);
            return data;
        }

        public static uint ReadUInt(byte[] buffer, int offset, int count)
        {
            uint data = buffer[offset];
            for (int i = 1; i < FixedWidth.UInt; i++)
            {
                data <<= 8;
                data |= buffer[offset + i];
            }
            return data;
        }

        public static long ReadLong(ByteBuffer buffer)
        {
            return (long)ReadULong(buffer);
        }

        public static ulong ReadULong(ByteBuffer buffer)
        {
            buffer.Validate(false, FixedWidth.ULong);
            ulong data = ReadULong(buffer.Buffer, buffer.Offset, buffer.Length);
            buffer.Complete(FixedWidth.ULong);
            return data;
        }

        public static ulong ReadULong(byte[] buffer, int offset, int count)
        {
            ulong data = buffer[offset];
            for (int i = 1; i < FixedWidth.ULong; i++)
            {
                data <<= 8;
                data |= buffer[offset + i];
            }
            return data;
        }

        public static float ReadFloat(ByteBuffer buffer)
        {
            uint data = ReadUInt(buffer);
            return *((float*)&data);
        }

        public static double ReadDouble(ByteBuffer buffer)
        {
            ulong data = ReadULong(buffer);
            return *((double*)&data);
        }

        public static Guid ReadUuid(ByteBuffer buffer)
        {
            uint a = ReadUInt(buffer);
            ushort b = ReadUShort(buffer);
            ushort c = ReadUShort(buffer);
            buffer.Validate(false, FixedWidth.ULong);
            byte d = buffer.Buffer[buffer.Offset];
            byte e = buffer.Buffer[buffer.Offset + 1];
            byte f = buffer.Buffer[buffer.Offset + 2];
            byte g = buffer.Buffer[buffer.Offset + 3];
            byte h = buffer.Buffer[buffer.Offset + 4];
            byte i = buffer.Buffer[buffer.Offset + 5];
            byte j = buffer.Buffer[buffer.Offset + 6];
            byte k = buffer.Buffer[buffer.Offset + 7];
            buffer.Complete(FixedWidth.ULong);
            return new Guid(a, b, c, d, e, f, g, h, i, j, k);
        }

        public static void ReadBytes(ByteBuffer buffer, byte[] data, int offset, int count)
        {
            buffer.Validate(false, count);
            Buffer.BlockCopy(buffer.Buffer, buffer.Offset, data, offset, count);
            buffer.Complete(count);
        }

        public static void WriteByte(ByteBuffer buffer, sbyte data)
        {
            buffer.Validate(true, FixedWidth.Byte);
            buffer.Buffer[buffer.WritePos] = (byte)data;
            buffer.Append(FixedWidth.Byte);
        }

        public static void WriteUByte(ByteBuffer buffer, byte data)
        {
            buffer.Validate(true, FixedWidth.UByte);
            buffer.Buffer[buffer.WritePos] = data;
            buffer.Append(FixedWidth.UByte);
        }

        public static void WriteUByte(byte[] buffer, int offset, byte data)
        {
            buffer[offset] = data;
        }

        public static void WriteShort(ByteBuffer buffer, short data)
        {
            WriteUShort(buffer, (ushort)data);
        }

        public static void WriteUShort(ByteBuffer buffer, ushort data)
        {
            buffer.Validate(true, FixedWidth.UShort);
            WriteUShort(buffer.Buffer, buffer.WritePos, data);
            buffer.Append(FixedWidth.UShort);
        }

        public static void WriteUShort(byte[] buffer, int offset, ushort data)
        {
            buffer[offset] = (byte)(data >> 8);
            buffer[offset + 1] = (byte)data;
        }

        public static void WriteInt(ByteBuffer buffer, int data)
        {
            WriteUInt(buffer, (uint)data);
        }

        public static void WriteUInt(ByteBuffer buffer, uint data)
        {
            buffer.Validate(true, FixedWidth.UInt);
            WriteUInt(buffer.Buffer, buffer.WritePos, data);
            buffer.Append(FixedWidth.UInt);
        }

        public static void WriteUInt(byte[] buffer, int offset, uint data)
        {
            buffer[offset] = (byte)(data >> 24);
            buffer[offset + 1] = (byte)(data >> 16);
            buffer[offset + 2] = (byte)(data >> 8);
            buffer[offset + 3] = (byte)data;
        }

        public static void WriteLong(ByteBuffer buffer, long data)
        {
            WriteULong(buffer, (ulong)data);
        }

        public static void WriteULong(ByteBuffer buffer, ulong data)
        {
            buffer.Validate(true, FixedWidth.ULong);
            WriteULong(buffer.Buffer, buffer.WritePos, data);
            buffer.Append(FixedWidth.ULong);
        }

        internal static void WriteULong(byte[] buffer, int offset, ulong data)
        {
            buffer[offset] = (byte)(data >> 56);
            buffer[offset + 1] = (byte)(data >> 48);
            buffer[offset + 2] = (byte)(data >> 40);
            buffer[offset + 3] = (byte)(data >> 32);
            buffer[offset + 4] = (byte)(data >> 24);
            buffer[offset + 5] = (byte)(data >> 16);
            buffer[offset + 6] = (byte)(data >> 8);
            buffer[offset + 7] = (byte)data;
        }

        public static void WriteFloat(ByteBuffer buffer, float data)
        {
            uint n = *((uint*)&data);
            WriteUInt(buffer, n);
        }

        public static void WriteDouble(ByteBuffer buffer, double data)
        {
            ulong n = *((ulong*)&data);
            WriteULong(buffer, n);
        }

        public static void WriteUuid(ByteBuffer buffer, Guid data)
        {
            buffer.Validate(true, FixedWidth.Uuid);
            byte* p = (byte*)&data;
            WriteUInt(buffer.Buffer, buffer.WritePos, *((uint*)p));
            WriteUShort(buffer.Buffer, buffer.WritePos + 4, *((ushort*)(p + 4)));
            WriteUShort(buffer.Buffer, buffer.WritePos + 6, *((ushort*)(p + 6)));
            for (int i = 8; i < FixedWidth.Uuid; i++)
            {
                buffer.Buffer[buffer.WritePos + i] = *(p + i);
            }
            buffer.Append(FixedWidth.Uuid);
        }

        public static void WriteBytes(ByteBuffer buffer, byte[] data, int offset, int count)
        {
            buffer.Validate(true, count);
            Buffer.BlockCopy(data, offset, buffer.Buffer, buffer.WritePos, count);
            buffer.Append(count);
        }
    }
}
