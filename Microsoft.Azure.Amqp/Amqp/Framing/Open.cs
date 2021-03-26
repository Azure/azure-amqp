// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    public class Open : Performative
    {
        public static readonly string Name = "amqp:open:list";
        public static readonly ulong Code = 0x0000000000000010;
        const int Fields = 10;

        // AMQP spec: To avoid spurious timeouts, the value in idle-time-out
        // SHOULD be half the peer’s actual timeout threshold.
        // Behavior of idle timeout depends on how it is set.
        // 1. If set by user, it is the actual timeout. On the protocol level
        //    half of that value is sent to the remote peer.
        // 2. If set by decoder, it is used as is.
        uint? idleTimeOut;
        bool isUserIdleTimeOut;

        public Open() : base(Name, Code)
        {
        }

        public string ContainerId { get; set; }

        public string HostName { get; set; }

        public uint? MaxFrameSize { get; set; }

        public ushort? ChannelMax { get; set; }

        public uint? IdleTimeOut
        {
            get { return this.idleTimeOut; }
            set { this.idleTimeOut = value; this.isUserIdleTimeOut = true; }
        }

        public Multiple<AmqpSymbol> OutgoingLocales { get; set; }

        public Multiple<AmqpSymbol> IncomingLocales { get; set; }

        public Multiple<AmqpSymbol> OfferedCapabilities { get; set; }

        public Multiple<AmqpSymbol> DesiredCapabilities { get; set; }

        public Fields Properties { get; set; }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        uint? ProtocolIdleTimeout
        {
            get { return this.isUserIdleTimeOut ? this.idleTimeOut / 2 : this.idleTimeOut; }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("open(");
            int count = 0;
            this.AddFieldToString(this.ContainerId != null, sb, "container-id", this.ContainerId, ref count);
            this.AddFieldToString(this.HostName != null, sb, "host-name", this.HostName, ref count);
            this.AddFieldToString(this.MaxFrameSize != null, sb, "max-frame-size", this.MaxFrameSize, ref count);
            this.AddFieldToString(this.ChannelMax != null, sb, "channel-max", this.ChannelMax, ref count);
            this.AddFieldToString(this.ProtocolIdleTimeout != null, sb, "idle-time-out", this.ProtocolIdleTimeout, ref count);
            this.AddFieldToString(this.OutgoingLocales != null, sb, "outgoing-locales", this.OutgoingLocales, ref count);
            this.AddFieldToString(this.IncomingLocales != null, sb, "incoming-locales", this.IncomingLocales, ref count);
            this.AddFieldToString(this.OfferedCapabilities != null, sb, "offered-capabilities", this.OfferedCapabilities, ref count);
            this.AddFieldToString(this.DesiredCapabilities != null, sb, "desired-capabilities", this.DesiredCapabilities, ref count);
            this.AddFieldToString(this.Properties != null, sb, "properties", this.Properties, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        protected override void EnsureRequired()
        {
            if (this.ContainerId == null)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "container-id", Name));
            }
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeString(this.ContainerId, buffer);
            AmqpCodec.EncodeString(this.HostName, buffer);
            AmqpCodec.EncodeUInt(this.MaxFrameSize, buffer);
            AmqpCodec.EncodeUShort(this.ChannelMax, buffer);
            AmqpCodec.EncodeUInt(this.ProtocolIdleTimeout, buffer);
            AmqpCodec.EncodeMultiple(this.OutgoingLocales, buffer);
            AmqpCodec.EncodeMultiple(this.IncomingLocales, buffer);
            AmqpCodec.EncodeMultiple(this.OfferedCapabilities, buffer);
            AmqpCodec.EncodeMultiple(this.DesiredCapabilities, buffer);
            AmqpCodec.EncodeMap(this.Properties, buffer);
        }

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
                this.idleTimeOut = AmqpCodec.DecodeUInt(buffer);
                this.isUserIdleTimeOut = false;
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

        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetStringEncodeSize(this.ContainerId);
            valueSize += AmqpCodec.GetStringEncodeSize(this.HostName);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.MaxFrameSize);
            valueSize += AmqpCodec.GetUShortEncodeSize(this.ChannelMax);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.ProtocolIdleTimeout);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.OutgoingLocales);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.IncomingLocales);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.OfferedCapabilities);
            valueSize += AmqpCodec.GetMultipleEncodeSize(this.DesiredCapabilities);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Properties);

            return valueSize;
        }

        internal void CopyIdleTimeOut(Open source)
        {
            this.idleTimeOut = source.idleTimeOut;
            this.isUserIdleTimeOut = source.isUserIdleTimeOut;
        }
    }
}
