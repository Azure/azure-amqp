// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the begin performative.
    /// </summary>
    public class Begin : Performative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:begin:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000011;
        const int Fields = 8;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Begin() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the "remote-channel" field.
        /// </summary>
        public ushort? RemoteChannel { get; set; }

        /// <summary>
        /// Gets or sets the "next-outgoing-id" field.
        /// </summary>
        public uint? NextOutgoingId { get; set; }

        /// <summary>
        /// Gets or sets the "incoming-window" field.
        /// </summary>
        public uint? IncomingWindow { get; set; }

        /// <summary>
        /// Gets or sets the "outgoing-window" field.
        /// </summary>
        public uint? OutgoingWindow { get; set; }

        /// <summary>
        /// Gets or sets the "handle-max" field.
        /// </summary>
        public uint? HandleMax { get; set; }

        /// <summary>
        /// Gets or sets the "offered-capabilities" field.
        /// </summary>
        public Multiple<AmqpSymbol> OfferedCapabilities { get; set; }

        /// <summary>
        /// Gets or sets the "desired-capabilities" field.
        /// </summary>
        public Multiple<AmqpSymbol> DesiredCapabilities { get; set; }

        /// <summary>
        /// Gets or sets the "properties" field.
        /// </summary>
        public Fields Properties { get; set; }

        /// <summary>
        /// Gets the number of fields in the list.
        /// </summary>
        protected override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("begin(");
            int count = 0;
            this.AddFieldToString(this.RemoteChannel != null, sb, "remote-channel", this.RemoteChannel, ref count);
            this.AddFieldToString(this.NextOutgoingId != null, sb, "next-outgoing-id", this.NextOutgoingId, ref count);
            this.AddFieldToString(this.IncomingWindow != null, sb, "incoming-window", this.IncomingWindow, ref count);
            this.AddFieldToString(this.OutgoingWindow != null, sb, "outgoing-window", this.OutgoingWindow, ref count);
            this.AddFieldToString(this.HandleMax != null, sb, "handle-max", this.HandleMax, ref count);
            this.AddFieldToString(this.OfferedCapabilities != null, sb, "offered-capabilities", this.OfferedCapabilities, ref count);
            this.AddFieldToString(this.DesiredCapabilities != null, sb, "desired-capabilities", this.DesiredCapabilities, ref count);
            this.AddFieldToString(this.Properties != null, sb, "properties", this.Properties, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (!this.NextOutgoingId.HasValue)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "begin.next-outgoing-id");
            }

            if (!this.IncomingWindow.HasValue)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "begin.incoming-window");
            }

            if (!this.OutgoingWindow.HasValue)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "begin.outgoing-window");
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUShort(this.RemoteChannel, buffer);
            AmqpCodec.EncodeUInt(this.NextOutgoingId, buffer);
            AmqpCodec.EncodeUInt(this.IncomingWindow, buffer);
            AmqpCodec.EncodeUInt(this.OutgoingWindow, buffer);
            AmqpCodec.EncodeUInt(this.HandleMax, buffer);
            AmqpCodec.EncodeMultiple(this.OfferedCapabilities, buffer);
            AmqpCodec.EncodeMultiple(this.DesiredCapabilities, buffer);
            AmqpCodec.EncodeMap(this.Properties, buffer);
        }

        /// <summary>
        /// Decodes the fields from the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        /// <param name="count">The number of fields.</param>
        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.RemoteChannel = AmqpCodec.DecodeUShort(buffer);
            }

            if (count-- > 0)
            {
                this.NextOutgoingId = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.IncomingWindow = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.OutgoingWindow = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.HandleMax = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.OfferedCapabilities = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }

            if (count-- > 0)
            {
                this.DesiredCapabilities = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }

            if (count-- > 0)
            {
                this.Properties = AmqpCodec.DecodeMap<Fields>(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetUShortEncodeSize(this.RemoteChannel);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.NextOutgoingId);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.IncomingWindow);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.OutgoingWindow);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.HandleMax);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.OfferedCapabilities);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.DesiredCapabilities);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Properties);

            return valueSize;
        }
    }
}
