// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the attach performative.
    /// </summary>
    public class Attach : LinkPerformative
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:attach:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000012;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Attach() : base(Name, Code, 14)
        {
        }

        /// <summary>
        /// Gets or sets the "name" field.
        /// </summary>
        public string LinkName { get; set; }

        /// <summary>
        /// Gets or sets the "role" field. True if it's a receiver.
        /// </summary>
        public bool? Role { get; set; }

        /// <summary>
        /// Gets or sets the "snd-settle-mode" field.
        /// </summary>
        public byte? SndSettleMode { get; set; }

        /// <summary>
        /// Gets or sets the "rcv-settle-mode" field.
        /// </summary>
        public byte? RcvSettleMode { get; set; }

        /// <summary>
        /// Gets or sets the "source" field.
        /// </summary>
        public object Source { get; set; }

        /// <summary>
        /// Gets or sets the "target" field.
        /// </summary>
        public object Target { get; set; }

        /// <summary>
        /// Gets or sets the "unsettled" field.
        /// </summary>
        public AmqpMap Unsettled { get; set; }

        /// <summary>
        /// Gets or sets the "incomplete-unsettled" field.
        /// </summary>
        public bool? IncompleteUnsettled { get; set; }

        /// <summary>
        /// Gets or sets the "initial-delivery-count" field.
        /// </summary>
        public uint? InitialDeliveryCount { get; set; }

        /// <summary>
        /// Gets or sets the "max-message-size" field.
        /// </summary>
        public ulong? MaxMessageSize { get; set; }

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
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("attach(");
            int count = 0;
            this.AddFieldToString(this.LinkName != null, sb, "name", this.LinkName, ref count);
            this.AddFieldToString(this.Handle != null, sb, "handle", this.Handle, ref count);
            this.AddFieldToString(this.Role != null, sb, "role", this.Role, ref count);
            this.AddFieldToString(this.SndSettleMode != null, sb, "snd-settle-mode", this.SndSettleMode, ref count);
            this.AddFieldToString(this.RcvSettleMode != null, sb, "rcv-settle-mode", this.RcvSettleMode, ref count);
            this.AddFieldToString(this.Source != null, sb, "source", this.Source, ref count);
            this.AddFieldToString(this.Target != null, sb, "target", this.Target, ref count);
            this.AddFieldToString(this.IncompleteUnsettled != null, sb, "incomplete-unsettled", this.IncompleteUnsettled, ref count);
            this.AddFieldToString(this.InitialDeliveryCount != null, sb, "initial-delivery-count", this.InitialDeliveryCount, ref count);
            this.AddFieldToString(this.MaxMessageSize != null, sb, "max-message-size", this.MaxMessageSize, ref count);
            this.AddFieldToString(this.OfferedCapabilities != null, sb, "offered-capabilities", this.OfferedCapabilities, ref count);
            this.AddFieldToString(this.DesiredCapabilities != null, sb, "desired-capabilities", this.DesiredCapabilities, ref count);
            this.AddFieldToString(this.Properties != null, sb, "properties", this.Properties, ref count);
            this.AddFieldToString(this.Unsettled != null, sb, "unsettled-map", this.Unsettled, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (this.LinkName == null)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "name", Name));
            }

            if (!this.Handle.HasValue)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "handle", Name));
            }

            if (!this.Role.HasValue)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "role", Name));
            }

            ////if (!this.Role.Value && this.InitialDeliveryCount == null)
            ////{
            ////    throw AmqpEncoding.GetEncodingException("attach.initial-delivery-count");
            ////}
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeString(this.LinkName, buffer);
            AmqpCodec.EncodeUInt(this.Handle, buffer);
            AmqpCodec.EncodeBoolean(this.Role, buffer);
            AmqpCodec.EncodeUByte(this.SndSettleMode, buffer);
            AmqpCodec.EncodeUByte(this.RcvSettleMode, buffer);
            AmqpCodec.EncodeObject(this.Source, buffer);
            AmqpCodec.EncodeObject(this.Target, buffer);
            AmqpCodec.EncodeMap(this.Unsettled, buffer);
            AmqpCodec.EncodeBoolean(this.IncompleteUnsettled, buffer);
            AmqpCodec.EncodeUInt(this.InitialDeliveryCount, buffer);
            AmqpCodec.EncodeULong(this.MaxMessageSize, buffer);
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
                this.LinkName = AmqpCodec.DecodeString(buffer);
            }

            if (count-- > 0)
            {
                this.Handle = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Role = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.SndSettleMode = AmqpCodec.DecodeUByte(buffer);
            }

            if (count-- > 0)
            {
                this.RcvSettleMode = AmqpCodec.DecodeUByte(buffer);
            }

            if (count-- > 0)
            {
                this.Source = AmqpCodec.DecodeObject(buffer);
            }

            if (count-- > 0)
            {
                this.Target = AmqpCodec.DecodeObject(buffer);
            }

            if (count-- > 0)
            {
                this.Unsettled = AmqpCodec.DecodeMap(buffer, MapKeyByteArrayComparer.Instance);
            }

            if (count-- > 0)
            {
                this.IncompleteUnsettled = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.InitialDeliveryCount = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.MaxMessageSize = AmqpCodec.DecodeULong(buffer);
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

            valueSize += AmqpCodec.GetStringEncodeSize(this.LinkName);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.Handle);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Role);
            valueSize += AmqpCodec.GetUByteEncodeSize(this.SndSettleMode);
            valueSize += AmqpCodec.GetUByteEncodeSize(this.RcvSettleMode);
            valueSize += AmqpCodec.GetObjectEncodeSize(this.Source);
            valueSize += AmqpCodec.GetObjectEncodeSize(this.Target);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Unsettled);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.IncompleteUnsettled);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.InitialDeliveryCount);
            valueSize += AmqpCodec.GetULongEncodeSize(this.MaxMessageSize);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.OfferedCapabilities);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.DesiredCapabilities);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Properties);

            return valueSize;
        }
    }
}
