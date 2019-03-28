// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Globalization;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the AMQP frame.
    /// </summary>
    public sealed class Frame : IDisposable
    {
        internal const int HeaderSize = 8;
        const byte DefaultDataOffset = 2;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Frame()
            : this(FrameType.Amqp)
        {
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="type">The frame type.</param>
        public Frame(FrameType type)
        {
            this.Type = type;
            this.DataOffset = Frame.DefaultDataOffset;
        }

        /// <summary>
        /// Gets the size of the frame.
        /// </summary>
        public int Size
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the data offset in the frame buffer.
        /// </summary>
        public byte DataOffset
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the frame type.
        /// </summary>
        public FrameType Type
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets or sets the session channel.
        /// </summary>
        public ushort Channel
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the performative of the frame.
        /// </summary>
        public Performative Command
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the payload, if any, of the performative.
        /// </summary>
        public ArraySegment<byte> Payload
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the buffer of the frame.
        /// </summary>
        public ByteBuffer RawByteBuffer
        {
            get; 
            private set; 
        }

        internal static ByteBuffer EncodeCommand(FrameType type, ushort channel, Performative command, int payloadSize)
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

        /// <summary>
        /// Serializes the frame. Payload is not copied to <see cref="RawByteBuffer"/>.
        /// </summary>
        public void Encode()
        {
            this.RawByteBuffer = EncodeCommand(this.Type, this.Channel, this.Command, this.Payload.Count);
        }

        /// <summary>
        /// Deserializes the frame.
        /// </summary>
        /// <param name="buffer">The frame buffer.</param>
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

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
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

        /// <summary>
        /// Disposes the frame and releases the buffer.
        /// </summary>
        public void Dispose()
        {
            if (this.RawByteBuffer != null)
            {
                this.RawByteBuffer.Dispose();
            }
        }
    }
}
