// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the properties message section.
    /// </summary>
    public sealed class Properties : DescribedList
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:properties:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000073;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Properties() : base(Name, Code, 13) { }

        /// <summary>
        /// Gets or sets the "message-id" field..
        /// </summary>
        public MessageId MessageId { get; set; }

        /// <summary>
        /// Gets or sets the "user-id" field.
        /// </summary>
        public ArraySegment<byte> UserId { get; set; }

        /// <summary>
        /// Gets or sets the "to" field.
        /// </summary>
        public Address To { get; set; }

        /// <summary>
        /// Gets or sets the "subject" field.
        /// </summary>
        public string Subject { get; set; }

        /// <summary>
        /// Gets or sets the "reply-to" field.
        /// </summary>
        public Address ReplyTo { get; set; }

        /// <summary>
        /// Gets or sets the "correlation-id" field.
        /// </summary>
        public MessageId CorrelationId { get; set; }

        /// <summary>
        /// Gets or sets the "content-type" field.
        /// </summary>
        public AmqpSymbol ContentType { get; set; }

        /// <summary>
        /// Gets or sets the "content-encoding" field.
        /// </summary>
        public AmqpSymbol ContentEncoding { get; set; }

        /// <summary>
        /// Gets or sets the "absolute-expiry-time" field.
        /// </summary>
        public DateTime? AbsoluteExpiryTime { get; set; }

        /// <summary>
        /// Gets or sets the "creation-time" field.
        /// </summary>
        public DateTime? CreationTime { get; set; }

        /// <summary>
        /// Gets or sets the "group-id" field.
        /// </summary>
        public string GroupId { get; set; }

        /// <summary>
        /// Gets or sets the "group-sequence" field.
        /// </summary>
        public uint? GroupSequence { get; set; }

        /// <summary>
        /// Gets or sets the "reply-to-group-id" field.
        /// </summary>
        public string ReplyToGroupId { get; set; }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("properties(");
            int count = 0;
            this.AddFieldToString(this.MessageId != null, sb, "message-id", this.MessageId, ref count);
            this.AddFieldToString(this.UserId.Array != null, sb, "user-id", this.UserId, ref count);
            this.AddFieldToString(this.To != null, sb, "to", this.To, ref count);
            this.AddFieldToString(this.Subject != null, sb, "subject", this.Subject, ref count);
            this.AddFieldToString(this.ReplyTo != null, sb, "reply-to", this.ReplyTo, ref count);
            this.AddFieldToString(this.CorrelationId != null, sb, "correlation-id", this.CorrelationId, ref count);
            this.AddFieldToString(this.ContentType.Value != null, sb, "content-type", this.ContentType, ref count);
            this.AddFieldToString(this.ContentEncoding.Value != null, sb, "content-encoding", this.ContentEncoding, ref count);
            this.AddFieldToString(this.AbsoluteExpiryTime != null, sb, "absolute-expiry-time", this.AbsoluteExpiryTime, ref count);
            this.AddFieldToString(this.CreationTime != null, sb, "creation-time", this.CreationTime, ref count);
            this.AddFieldToString(this.GroupId != null, sb, "group-id", this.GroupId, ref count);
            this.AddFieldToString(this.GroupSequence != null, sb, "group-sequence", this.GroupSequence, ref count);
            this.AddFieldToString(this.ReplyToGroupId != null, sb, "reply-to-group-id", this.ReplyToGroupId, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            MessageId.Encode(buffer, this.MessageId);
            AmqpCodec.EncodeBinary(this.UserId, buffer);
            Address.Encode(buffer, this.To);
            AmqpCodec.EncodeString(this.Subject, buffer);
            Address.Encode(buffer, this.ReplyTo);
            MessageId.Encode(buffer, this.CorrelationId);
            AmqpCodec.EncodeSymbol(this.ContentType, buffer);
            AmqpCodec.EncodeSymbol(this.ContentEncoding, buffer);
            AmqpCodec.EncodeTimeStamp(this.AbsoluteExpiryTime, buffer);
            AmqpCodec.EncodeTimeStamp(this.CreationTime, buffer);
            AmqpCodec.EncodeString(this.GroupId, buffer);
            AmqpCodec.EncodeUInt(this.GroupSequence, buffer);
            AmqpCodec.EncodeString(this.ReplyToGroupId, buffer);
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
                this.MessageId = MessageId.Decode(buffer);
            }

            if (count-- > 0)
            {
                this.UserId = AmqpCodec.DecodeBinary(buffer);
            }

            if (count-- > 0)
            {
                this.To = Address.Decode(buffer);
            }

            if (count-- > 0)
            {
                this.Subject = AmqpCodec.DecodeString(buffer);
            }

            if (count-- > 0)
            {
                this.ReplyTo = Address.Decode(buffer);
            }

            if (count-- > 0)
            {
                this.CorrelationId = MessageId.Decode(buffer);
            }

            if (count-- > 0)
            {
                this.ContentType = AmqpCodec.DecodeSymbol(buffer);
            }

            if (count-- > 0)
            {
                this.ContentEncoding = AmqpCodec.DecodeSymbol(buffer);
            }

            if (count-- > 0)
            {
                this.AbsoluteExpiryTime = AmqpCodec.DecodeTimeStamp(buffer);
            }

            if (count-- > 0)
            {
                this.CreationTime = AmqpCodec.DecodeTimeStamp(buffer);
            }

            if (count-- > 0)
            {
                this.GroupId = AmqpCodec.DecodeString(buffer);
            }

            if (count-- > 0)
            {
                this.GroupSequence = AmqpCodec.DecodeUInt(buffer);
            }

            if (count-- > 0)
            {
                this.ReplyToGroupId = AmqpCodec.DecodeString(buffer);
            }
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize = MessageId.GetEncodeSize(this.MessageId);
            valueSize += AmqpCodec.GetBinaryEncodeSize(this.UserId);
            valueSize += Address.GetEncodeSize(this.To);
            valueSize += AmqpCodec.GetStringEncodeSize(this.Subject);
            valueSize += Address.GetEncodeSize(this.ReplyTo);
            valueSize += MessageId.GetEncodeSize(this.CorrelationId);
            valueSize += AmqpCodec.GetSymbolEncodeSize(this.ContentType);
            valueSize += AmqpCodec.GetSymbolEncodeSize(this.ContentEncoding);
            valueSize += AmqpCodec.GetTimeStampEncodeSize(this.AbsoluteExpiryTime);
            valueSize += AmqpCodec.GetTimeStampEncodeSize(this.CreationTime);
            valueSize += AmqpCodec.GetStringEncodeSize(this.GroupId);
            valueSize += AmqpCodec.GetUIntEncodeSize(this.GroupSequence);
            valueSize += AmqpCodec.GetStringEncodeSize(this.ReplyToGroupId);

            return valueSize;
        }

    }
}
