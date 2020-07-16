// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the flow performative.
    /// </summary>
    public sealed class Flow : LinkPerformative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:flow:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000013;
        const int Fields = 11;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Flow() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the "next-incoming-id" field.
        /// </summary>
        public uint? NextIncomingId { get; set; }

        /// <summary>
        /// Gets or sets the "incoming-window" field.
        /// </summary>
        public uint? IncomingWindow { get; set; }

        /// <summary>
        /// Gets or sets the "next-outgoing-id" field.
        /// </summary>
        public uint? NextOutgoingId { get; set; }

        /// <summary>
        /// Gets or sets the "outgoing-window" field.
        /// </summary>
        public uint? OutgoingWindow { get; set; }

        /// <summary>
        /// Gets or sets the "delivery-count" field.
        /// </summary>
        public uint? DeliveryCount { get; set; }

        /// <summary>
        /// Gets or sets the "link-credit" field.
        /// </summary>
        public uint? LinkCredit { get; set; }

        /// <summary>
        /// Gets or sets the "available" field.
        /// </summary>
        public uint? Available { get; set; }

        /// <summary>
        /// Gets or sets the "drain" field.
        /// </summary>
        public bool? Drain { get; set; }

        /// <summary>
        /// Gets or sets the "echo" field.
        /// </summary>
        public bool? Echo { get; set; }

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
            StringBuilder sb = new StringBuilder("flow(");
            int count = 0;
            this.AddFieldToString(this.NextIncomingId != null, sb, "next-in-id", this.NextIncomingId, ref count);
            this.AddFieldToString(this.IncomingWindow != null, sb, "in-window", this.IncomingWindow, ref count);
            this.AddFieldToString(this.NextOutgoingId != null, sb, "next-out-id", this.NextOutgoingId, ref count);
            this.AddFieldToString(this.OutgoingWindow != null, sb, "out-window", this.OutgoingWindow, ref count);
            this.AddFieldToString(this.Handle != null, sb, "handle", this.Handle, ref count);
            this.AddFieldToString(this.LinkCredit != null, sb, "link-credit", this.LinkCredit, ref count);
            this.AddFieldToString(this.DeliveryCount != null, sb, "delivery-count", this.DeliveryCount, ref count);
            this.AddFieldToString(this.Available != null, sb, "available", this.Available, ref count);
            this.AddFieldToString(this.Drain != null, sb, "drain", this.Drain, ref count);
            this.AddFieldToString(this.Echo != null, sb, "echo", this.Echo, ref count);
            this.AddFieldToString(this.Properties != null, sb, "properties", this.Properties, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (!this.IncomingWindow.HasValue)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "flow.incoming-window");
            }

            if (!this.NextOutgoingId.HasValue)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "flow.next-outgoing-id");
            }

            if (!this.OutgoingWindow.HasValue)
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, "flow.outgoing-window");
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUInt(this.NextIncomingId, buffer);
            AmqpCodec.EncodeUInt(this.IncomingWindow, buffer);
            AmqpCodec.EncodeUInt(this.NextOutgoingId, buffer);
            AmqpCodec.EncodeUInt(this.OutgoingWindow, buffer);
            AmqpCodec.EncodeUInt(this.Handle, buffer);
            AmqpCodec.EncodeUInt(this.DeliveryCount, buffer);
            AmqpCodec.EncodeUInt(this.LinkCredit, buffer);
            AmqpCodec.EncodeUInt(this.Available, buffer);
            AmqpCodec.EncodeBoolean(this.Drain, buffer);
            AmqpCodec.EncodeBoolean(this.Echo, buffer);
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
                this.NextIncomingId = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.IncomingWindow = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.NextOutgoingId = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.OutgoingWindow = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Handle = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.DeliveryCount = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.LinkCredit = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Available = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Drain = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.Echo = AmqpCodec.DecodeBoolean(buffer);
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

            valueSize += AmqpCodec.GetUIntEncodeSize(this.NextIncomingId);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.IncomingWindow);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.NextOutgoingId);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.OutgoingWindow);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Handle);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.DeliveryCount);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.LinkCredit);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Available);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Drain);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Echo);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Properties);

            return valueSize;
        }
    }
}
