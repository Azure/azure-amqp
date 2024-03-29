﻿// Copyright (c) Microsoft. All rights reserved.
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
        public const string Name = "amqp:detach:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000016;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Detach() : base(Name, Code, 3)
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

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeUInt(this.Handle, buffer);
            AmqpCodec.EncodeBoolean(this.Closed, buffer);
            AmqpCodec.EncodeSerializable(this.Error, buffer);
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

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
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
