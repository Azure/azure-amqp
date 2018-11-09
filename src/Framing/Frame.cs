// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Globalization;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Frame : IDisposable
    {
        public const int HeaderSize = 8;
        const byte DefaultDataOffset = 2;

        public Frame()
            : this(FrameType.Amqp)
        {
        }

        public Frame(FrameType type)
        {
            this.Type = type;
            this.DataOffset = Frame.DefaultDataOffset;
        }

        public int Size
        {
            get;
            private set;
        }

        public byte DataOffset
        {
            get;
            private set;
        }

        public FrameType Type
        {
            get;
            private set;
        }

        public ushort Channel
        {
            get;
            set;
        }

        public Performative Command
        {
            get;
            set;
        }

        public ArraySegment<byte> Payload
        {
            get;
            set;
        }

        public ByteBuffer RawByteBuffer
        {
            get; 
            private set; 
        }

        public static ByteBuffer EncodeCommand(FrameType type, ushort channel, Performative command, int payloadSize)
        {
            int frameSize = HeaderSize;
            if (command != null)
            {
                frameSize += AmqpCodec.GetSerializableEncodeSize(command);
            }

            frameSize += payloadSize;

            ByteBuffer buffer = new ByteBuffer(frameSize, false, false);
            AmqpBitConverter.WriteUInt(buffer, (uint)frameSize);
            AmqpBitConverter.WriteUByte(buffer, DefaultDataOffset);
            AmqpBitConverter.WriteUByte(buffer, (byte)type);
            AmqpBitConverter.WriteUShort(buffer, channel);

            if (command != null)
            {
                AmqpCodec.EncodeSerializable(command, buffer);
            }

            return buffer;
        }

        public void Decode(ByteBuffer buffer)
        {
            // the frame now owns disposing the buffer
            this.RawByteBuffer = buffer;

            int offset = buffer.Offset;
            int length = buffer.Length;
            this.DecodeHeader(buffer);
            this.DecodeCommand(buffer);
            this.DecodePayload(buffer);
            buffer.AdjustPosition(offset, length);
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.InvariantCulture, "FRM({0:X4}|{1}|{2}|{3:X2}", this.Size, this.DataOffset, (byte)this.Type, this.Channel);
            if (this.Command != null)
            {
                sb.AppendFormat(CultureInfo.InvariantCulture, "  {0}", this.Command);
            }

            if (this.Payload.Count > 0)
            {
                sb.AppendFormat(CultureInfo.InvariantCulture, ",{0}", this.Payload.Count);
            }

            sb.Append(')');
            return sb.ToString();
        }

        void DecodeHeader(ByteBuffer buffer)
        {
            this.Size = (int)AmqpBitConverter.ReadUInt(buffer);
            this.DataOffset = AmqpBitConverter.ReadUByte(buffer);
            this.Type = (FrameType)AmqpBitConverter.ReadUByte(buffer);
            this.Channel = AmqpBitConverter.ReadUShort(buffer);

            // skip extended header
            buffer.Complete(this.DataOffset * 4 - Frame.HeaderSize);
        }

        void DecodeCommand(ByteBuffer buffer)
        {
            if (buffer.Length > 0)
            {
                this.Command = (Performative)AmqpCodec.DecodeAmqpDescribed(buffer);
            }
        }

        void DecodePayload(ByteBuffer buffer)
        {
            if (buffer.Length > 0)
            {
                this.Payload = new ArraySegment<byte>(buffer.Buffer, buffer.Offset, buffer.Length);
            }
        }

        public void Dispose()
        {
            if (this.RawByteBuffer != null)
            {
                this.RawByteBuffer.Dispose();
            }
        }
    }
}
