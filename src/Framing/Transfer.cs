// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the transfer performative.
    /// </summary>
    public sealed class Transfer : LinkPerformative
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:transfer:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000014;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Transfer() : base(Name, Code, 11) { }

        /// <summary>
        /// Gets or sets the "delivery-id" field.
        /// </summary>
        public uint? DeliveryId { get; set; }

        /// <summary>
        /// Gets or sets the "delivery-tag" field.
        /// </summary>
        public ArraySegment<byte> DeliveryTag { get; set; }

        /// <summary>
        /// Gets or sets the "message-format" field.
        /// </summary>
        public uint? MessageFormat { get; set; }

        /// <summary>
        /// Gets or sets the "settled" field.
        /// </summary>
        public bool? Settled { get; set; }

        /// <summary>
        /// Gets or sets the "more" field.
        /// </summary>
        public bool? More { get; set; }

        /// <summary>
        /// Gets or sets the "rcv-settle-mode" field.
        /// </summary>
        public byte? RcvSettleMode { get; set; }

        /// <summary>
        /// Gets or sets the "state" field.
        /// </summary>
        public DeliveryState State { get; set; }

        /// <summary>
        /// Gets or sets the "resume" field.
        /// </summary>
        public bool? Resume { get; set; }

        /// <summary>
        /// Gets or sets the "aborted" field.
        /// </summary>
        public bool? Aborted { get; set; }

        /// <summary>
        /// Gets or sets the "batchable" field.
        /// </summary>
        public bool? Batchable { get; set; }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("transfer(");
            int count = 0;
            this.AddFieldToString(this.Handle != null, sb, "handle", this.Handle, ref count);
            this.AddFieldToString(this.DeliveryId != null, sb, "delivery-id", this.DeliveryId, ref count);
            this.AddFieldToString(this.DeliveryTag.Array != null, sb, "delivery-tag", this.DeliveryTag, ref count);
            this.AddFieldToString(this.MessageFormat != null, sb, "message-format", this.MessageFormat, ref count);
            this.AddFieldToString(this.Settled != null, sb, "settled", this.Settled, ref count);
            this.AddFieldToString(this.More != null, sb, "more", this.More, ref count);
            this.AddFieldToString(this.RcvSettleMode != null, sb, "rcv-settle-mode", this.RcvSettleMode, ref count);
            this.AddFieldToString(this.State != null, sb, "state", this.State, ref count);
            this.AddFieldToString(this.Resume != null, sb, "resume", this.Resume, ref count);
            this.AddFieldToString(this.Aborted != null, sb, "aborted", this.Aborted, ref count);
            this.AddFieldToString(this.Batchable != null, sb, "batchable", this.Batchable, ref count);
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
            AmqpCodec.EncodeUInt(this.DeliveryId, buffer);
            AmqpCodec.EncodeBinary(this.DeliveryTag, buffer);
            AmqpCodec.EncodeUInt(this.MessageFormat, buffer);
            AmqpCodec.EncodeBoolean(this.Settled, buffer);
            AmqpCodec.EncodeBoolean(this.More, buffer);
            AmqpCodec.EncodeUByte(this.RcvSettleMode, buffer);
            AmqpCodec.EncodeSerializable(this.State, buffer);
            AmqpCodec.EncodeBoolean(this.Resume, buffer);
            AmqpCodec.EncodeBoolean(this.Aborted, buffer);
            AmqpCodec.EncodeBoolean(this.Batchable, buffer);
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
                this.DeliveryId = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.DeliveryTag = AmqpCodec.DecodeBinary(buffer);
            }

            if (count-- > 0)
            {
                this.MessageFormat = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.Settled = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.More = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.RcvSettleMode = AmqpCodec.DecodeUByte(buffer);
            }

            if (count-- > 0)
            {
                this.State = (DeliveryState)AmqpCodec.DecodeAmqpDescribed(buffer);
            }

            if (count-- > 0)
            {
                this.Resume = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.Aborted = AmqpCodec.DecodeBoolean(buffer);
            }

            if (count-- > 0)
            {
                this.Batchable = AmqpCodec.DecodeBoolean(buffer);
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
            valueSize += AmqpCodec.GetUIntEncodeSize(this.DeliveryId);
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.DeliveryTag);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.MessageFormat);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Settled);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.More);
            valueSize += AmqpCodec.GetUByteEncodeSize(this.RcvSettleMode);
            valueSize += AmqpCodec.GetSerializableEncodeSize(this.State);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Resume);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Aborted);
            valueSize += AmqpCodec.GetBooleanEncodeSize(this.Batchable);

            return valueSize;
        }
    }
}
