// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;

    public sealed class Received : DeliveryState
    {
        public static readonly string Name = "amqp:received:list";
        public static readonly ulong Code = 0x0000000000000023;
        const int Fields = 2;

        public Received() : base(Name, Code) { }

        public uint? SectionNumber { get; set; }

        public ulong? SectionOffset { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("received(");
            int count = 0;
            this.AddFieldToString(this.SectionNumber != null, sb, "section-number", this.SectionNumber, ref count);
            this.AddFieldToString(this.SectionOffset != null, sb, "section-offset", this.SectionOffset, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUInt(this.SectionNumber, buffer);
            AmqpCodec.EncodeULong(this.SectionOffset, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.SectionNumber = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.SectionOffset = AmqpCodec.DecodeULong(buffer);
            }
        }

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetUIntEncodeSize(this.SectionNumber);
            valueSize += AmqpCodec.GetULongEncodeSize(this.SectionOffset);

            return valueSize;
        }
    }
}
