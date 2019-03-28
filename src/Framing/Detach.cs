// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the detach performative.
    /// </summary>
    public sealed class Detach : LinkPerformative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:detach:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000016;
        const int Fields = 3;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Detach() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the "closed" field.
        /// </summary>
        public bool? Closed { get; set; }

        /// <summary>
        /// Gets or sets the "error" field.
        /// </summary>
        public Error Error { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
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

        internal override void EnsureRequired()
        {
            if (!this.Handle.HasValue)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "handle", Name));
            }
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUInt(this.Handle, buffer);
            AmqpCodec.EncodeBoolean(this.Closed, buffer);
            AmqpCodec.EncodeSerializable(this.Error, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
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

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetUIntEncodeSize(this.Handle);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Closed);
            valueSize += AmqpCodec.GetObjectEncodeSize(this.Error);

            return valueSize;
        }
    }
}
