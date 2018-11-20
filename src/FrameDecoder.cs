// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    sealed class FrameDecoder
    {
        int maxFrameSize;
        ByteBuffer currentFrameBuffer;

        public FrameDecoder(int maxFrameSize)
        {
            this.maxFrameSize = maxFrameSize;
        }

        public ProtocolHeader ExtractProtocolHeader(ByteBuffer buffer)
        {
            if (buffer.Length < AmqpConstants.ProtocolHeaderSize)
            {
                return null;
            }

            ProtocolHeader header = new ProtocolHeader();
            header.Decode(buffer);

            return header;
        }

        public void ExtractFrameBuffers(ByteBuffer buffer, SerializedWorker<ByteBuffer> bufferHandler)
        {
            if (this.currentFrameBuffer != null)
            {
                int sizeToWrite = Math.Min(this.currentFrameBuffer.Size, buffer.Length);

                AmqpBitConverter.WriteBytes(this.currentFrameBuffer, buffer.Buffer, buffer.Offset, sizeToWrite);
                buffer.Complete(sizeToWrite);

                if (this.currentFrameBuffer.Size == 0)
                {
                    ByteBuffer frameBuffer = this.currentFrameBuffer;
                    this.currentFrameBuffer = null;
                    bufferHandler.DoWork(frameBuffer);
                }
            }

            while (buffer.Length >= AmqpCodec.MinimumFrameDecodeSize)
            {
                int frameSize = AmqpCodec.GetFrameSize(buffer);
                if (frameSize < AmqpCodec.MinimumFrameDecodeSize || frameSize > this.maxFrameSize)
                {
                    throw new AmqpException(AmqpErrorCode.FramingError, CommonResources.GetString(CommonResources.InvalidFrameSize, frameSize, this.maxFrameSize));
                }

                int sizeToWrite = Math.Min(frameSize, buffer.Length);
                this.currentFrameBuffer = new ByteBuffer(frameSize, false);
                AmqpBitConverter.WriteBytes(this.currentFrameBuffer, buffer.Buffer, buffer.Offset, sizeToWrite);
                buffer.Complete(sizeToWrite);

                if (frameSize == sizeToWrite)
                {
                    ByteBuffer frameBuffer = this.currentFrameBuffer;
                    this.currentFrameBuffer = null;
                    bufferHandler.DoWork(frameBuffer);
                }
                else
                {
                    break;
                }
            }
        }
    }
}