// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Globalization;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the AMQP protocol header which consists of a protocol id and a version.
    /// </summary>
    public sealed class ProtocolHeader : IAmqpSerializable
    {
        const uint AmqpPrefix = 0x414D5150;

        ProtocolId protocolId;
        AmqpVersion version;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public ProtocolHeader()
        {
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="id">The protocol id.</param>
        /// <param name="version">The protocol version.</param>
        public ProtocolHeader(ProtocolId id, AmqpVersion version)
        {
            this.protocolId = id;
            this.version = version;
        }

        /// <summary>
        /// Gets the protocol id.
        /// </summary>
        public ProtocolId ProtocolId
        {
            get { return this.protocolId; }
        }

        /// <summary>
        /// Gets the protocol version.
        /// </summary>
        public AmqpVersion Version
        {
            get { return this.version; }
        }

        int IAmqpSerializable.EncodeSize
        {
            get
            {
                return AmqpConstants.ProtocolHeaderSize;
            }
        }

        void IAmqpSerializable.Encode(ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUInt(buffer, AmqpPrefix);
            AmqpBitConverter.WriteUByte(buffer, (byte)this.protocolId);
            AmqpBitConverter.WriteUByte(buffer, this.version.Major);
            AmqpBitConverter.WriteUByte(buffer, this.version.Minor);
            AmqpBitConverter.WriteUByte(buffer, this.version.Revision);
        }

        void IAmqpSerializable.Decode(ByteBuffer buffer)
        {
            if (buffer.Length < AmqpConstants.ProtocolHeaderSize)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, AmqpResources.GetString(AmqpResources.AmqpInsufficientBufferSize, AmqpConstants.ProtocolHeaderSize, buffer.Length));
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

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "AMQP {0} {1}", (byte)this.protocolId, this.version);
        }

        /// <summary>
        /// Determines whether two objects are equal.
        /// </summary>
        /// <param name="obj">The object to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
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

        /// <summary>
        /// Returns a hash code of the object.
        /// </summary>
        /// <returns>The hash code.</returns>
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
