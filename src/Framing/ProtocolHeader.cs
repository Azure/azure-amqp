// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using Microsoft.Azure.Amqp.Encoding;


    public sealed class ProtocolHeader : IAmqpSerializable
    {
        public static readonly ProtocolHeader Amqp100 = new ProtocolHeader(ProtocolId.Amqp, new AmqpVersion(1, 0, 0));
        public static readonly ProtocolHeader AmqpTls100 = new ProtocolHeader(ProtocolId.AmqpTls, new AmqpVersion(1, 0, 0));
        public static readonly ProtocolHeader AmqpSasl100 = new ProtocolHeader(ProtocolId.AmqpSasl, new AmqpVersion(1, 0, 0));

        const uint AmqpPrefix = 0x414D5150;

        ProtocolId protocolId;
        AmqpVersion version;

        public ProtocolHeader()
        {
        }

        public ProtocolHeader(ProtocolId id, AmqpVersion version)
        {
            this.protocolId = id;
            this.version = version;
        }

        public ProtocolId ProtocolId
        {
            get { return this.protocolId; }
        }

        public AmqpVersion Version
        {
            get { return this.version; }
        }

        public int EncodeSize
        {
            get
            {
                return AmqpConstants.ProtocolHeaderSize;
            }
        }

        public void Encode(ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUInt(buffer, AmqpPrefix);
            AmqpBitConverter.WriteUByte(buffer, (byte)this.protocolId);
            AmqpBitConverter.WriteUByte(buffer, this.version.Major);
            AmqpBitConverter.WriteUByte(buffer, this.version.Minor);
            AmqpBitConverter.WriteUByte(buffer, this.version.Revision);
        }

        public void Decode(ByteBuffer buffer)
        {
            if (buffer.Length < this.EncodeSize)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, AmqpResources.GetString(AmqpResources.AmqpInsufficientBufferSize, this.EncodeSize, buffer.Length));
            }

            uint prefix = AmqpBitConverter.ReadUInt(buffer);
            if (prefix != ProtocolHeader.AmqpPrefix)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, "ProtocolName" + prefix.ToString("X8"));
            }

            this.protocolId = (ProtocolId)AmqpBitConverter.ReadUByte(buffer);

            this.version = new AmqpVersion(
                AmqpBitConverter.ReadUByte(buffer),
                AmqpBitConverter.ReadUByte(buffer),
                AmqpBitConverter.ReadUByte(buffer));
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "AMQP {0} {1}", (byte)this.protocolId, this.version);
        }

        public override bool Equals(object obj)
        {
            ProtocolHeader otherHeader = obj as ProtocolHeader;
            if (otherHeader == null)
            {
                return false;
            }

            return otherHeader.protocolId == this.protocolId &&
                otherHeader.version.Equals(this.version);
        }

        public override int GetHashCode()
        {
            int result = ((int)this.protocolId << 24) +
                (this.version.Major << 16) +
                (this.version.Minor << 8) +
                this.version.Revision;
            return result.GetHashCode();
        }
    }
}
