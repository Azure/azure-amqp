// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the open performative.
    /// </summary>
    public class Open : Performative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:open:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000010;
        const int Fields = 10;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Open() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the "container-id" field.
        /// </summary>
        public string ContainerId { get; set; }

        /// <summary>
        /// Gets or sets the "hostname" field.
        /// </summary>
        public string HostName { get; set; }

        /// <summary>
        /// Gets or sets the "max-frame-size" field.
        /// </summary>
        public uint? MaxFrameSize { get; set; }

        /// <summary>
        /// Gets or sets the "channel-max" field.
        /// </summary>
        public ushort? ChannelMax { get; set; }

        /// <summary>
        /// Gets or sets the "idle-time-out" field.
        /// </summary>
        public uint? IdleTimeOut { get; set; }

        /// <summary>
        /// Gets or sets the "outgoing-locales" field.
        /// </summary>
        public Multiple<AmqpSymbol> OutgoingLocales { get; set; }

        /// <summary>
        /// Gets or sets the "incoming-locales" field.
        /// </summary>
        public Multiple<AmqpSymbol> IncomingLocales { get; set; }

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
            StringBuilder sb = new StringBuilder("open(");
            int count = 0;
            this.AddFieldToString(this.ContainerId != null, sb, "container-id", this.ContainerId, ref count);
            this.AddFieldToString(this.HostName != null, sb, "host-name", this.HostName, ref count);
            this.AddFieldToString(this.MaxFrameSize != null, sb, "max-frame-size", this.MaxFrameSize, ref count);
            this.AddFieldToString(this.ChannelMax != null, sb, "channel-max", this.ChannelMax, ref count);
            this.AddFieldToString(this.IdleTimeOut != null, sb, "idle-time-out", this.IdleTimeOut, ref count);
            this.AddFieldToString(this.OutgoingLocales != null, sb, "outgoing-locales", this.OutgoingLocales, ref count);
            this.AddFieldToString(this.IncomingLocales != null, sb, "incoming-locales", this.IncomingLocales, ref count);
            this.AddFieldToString(this.OfferedCapabilities != null, sb, "offered-capabilities", this.OfferedCapabilities, ref count);
            this.AddFieldToString(this.DesiredCapabilities != null, sb, "desired-capabilities", this.DesiredCapabilities, ref count);
            this.AddFieldToString(this.Properties != null, sb, "properties", this.Properties, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void EnsureRequired()
        {
            if (this.ContainerId == null)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "container-id", Name));
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeString(this.ContainerId, buffer);
            AmqpCodec.EncodeString(this.HostName, buffer);
            AmqpCodec.EncodeUInt(this.MaxFrameSize, buffer);
            AmqpCodec.EncodeUShort(this.ChannelMax, buffer);
            AmqpCodec.EncodeUInt(this.IdleTimeOut, buffer);
            AmqpCodec.EncodeMultiple(this.OutgoingLocales, buffer);
            AmqpCodec.EncodeMultiple(this.IncomingLocales, buffer);
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
                this.ContainerId = AmqpCodec.DecodeString(buffer);
            }

            if (count-- > 0)
            {
                this.HostName = AmqpCodec.DecodeString(buffer);
            }

            if (count-- > 0)
            {
                this.MaxFrameSize = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.ChannelMax = AmqpCodec.DecodeUShort(buffer);
            }

            if (count-- > 0)
            {
                this.IdleTimeOut = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.OutgoingLocales = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
            }

            if (count-- > 0)
            {
                this.IncomingLocales = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);
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

            valueSize += AmqpCodec.GetStringEncodeSize(this.ContainerId);
            valueSize += AmqpCodec.GetStringEncodeSize(this.HostName);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.MaxFrameSize);
            valueSize += AmqpCodec.GetUShortEncodeSize(this.ChannelMax);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.IdleTimeOut);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.OutgoingLocales);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.IncomingLocales);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.OfferedCapabilities);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.DesiredCapabilities);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Properties);

            return valueSize;
        }
    }
}
