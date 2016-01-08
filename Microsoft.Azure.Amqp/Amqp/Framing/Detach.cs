// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Detach : LinkPerformative
    {
        public static readonly string Name = "amqp:detach:list";
        public static readonly ulong Code = 0x0000000000000016;
        const int Fields = 3;

        public Detach() : base(Name, Code)
        {
        }

        //// public uint? Handle { get; set; }

        public bool? Closed { get; set; }

        public Error Error { get; set; }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("detach(");
            int count = 0;
            this.AddFieldToString(this.Handle != null, sb, "handle", this.Handle, ref count);
            this.AddFieldToString(this.Closed != null, sb, "closed", this.Closed, ref count);
            this.AddFieldToString(this.Error != null, sb, "error", this.Error, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        protected override void EnsureRequired()
        {
            if (!this.Handle.HasValue)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "handle", Name));
            }
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUInt(this.Handle, buffer);
            AmqpCodec.EncodeBoolean(this.Closed, buffer);
            AmqpCodec.EncodeSerializable(this.Error, buffer);
        }

        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Handle = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Closed = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.Error = AmqpCodec.DecodeKnownType<Error>(buffer);
            }
        }

        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetUIntEncodeSize(this.Handle);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Closed);
            valueSize += AmqpCodec.GetObjectEncodeSize(this.Error);

            return valueSize;
        }
    }
}
