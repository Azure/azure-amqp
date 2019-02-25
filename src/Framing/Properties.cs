// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Properties : DescribedList
    {
        public static readonly string Name = "amqp:properties:list";
        public static readonly ulong Code = 0x0000000000000073;
        public static readonly string MessageIdName = "message-id";
        public static readonly string UserIdName = "user-id";
        public static readonly string ToName = "to";
        public static readonly string SubjectName = "subject";
        public static readonly string ReplyToName = "reply-to";
        public static readonly string CorrelationIdName = "correlation-id";
        public static readonly string ContentTypeName = "content-type";
        public static readonly string ContentEncodingName = "content-encoding";
        public static readonly string AbsoluteExpiryTimeName = "absolute-expiry-time";
        public static readonly string CreationTimeName = "creation-time";
        public static readonly string GroupIdName = "group-id";
        public static readonly string GroupSequenceName = "group-sequence";
        public static readonly string ReplyToGroupIdName = "reply-to-group-id";

        const int Fields = 13;

        public Properties() : base(Name, Code) { }

        public MessageId MessageId { get; set; }

        public ArraySegment<byte> UserId { get; set; }

        public Address To { get; set; }

        public string Subject { get; set; }

        public Address ReplyTo { get; set; }

        public MessageId CorrelationId { get; set; }

        public AmqpSymbol ContentType { get; set; }

        public AmqpSymbol ContentEncoding { get; set; }

        public DateTime? AbsoluteExpiryTime { get; set; }

        public DateTime? CreationTime { get; set; }

        public string GroupId { get; set; }

        public uint? GroupSequence { get; set; }

        public string ReplyToGroupId { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("properties(");
            int count = 0;
            this.AddFieldToString(this.MessageId != null, sb, MessageIdName, this.MessageId, ref count);
            this.AddFieldToString(this.UserId.Array != null, sb, UserIdName, this.UserId, ref count);
            this.AddFieldToString(this.To != null, sb, ToName, this.To, ref count);
            this.AddFieldToString(this.Subject != null, sb, SubjectName, this.Subject, ref count);
            this.AddFieldToString(this.ReplyTo != null, sb, ReplyToName, this.ReplyTo, ref count);
            this.AddFieldToString(this.CorrelationId != null, sb, CorrelationIdName, this.CorrelationId, ref count);
            this.AddFieldToString(this.ContentType.Value != null, sb, ContentTypeName, this.ContentType, ref count);
            this.AddFieldToString(this.ContentEncoding.Value != null, sb, ContentEncodingName, this.ContentEncoding, ref count);
            this.AddFieldToString(this.AbsoluteExpiryTime != null, sb, AbsoluteExpiryTimeName, this.AbsoluteExpiryTime, ref count);
            this.AddFieldToString(this.CreationTime != null, sb, CreationTimeName, this.CreationTime, ref count);
            this.AddFieldToString(this.GroupId != null, sb, GroupIdName, this.GroupId, ref count);
            this.AddFieldToString(this.GroupSequence != null, sb, GroupSequenceName, this.GroupSequence, ref count);
            this.AddFieldToString(this.ReplyToGroupId != null, sb, ReplyToGroupIdName, this.ReplyToGroupId, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void OnEncode(ByteBuffer buffer)
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

        internal override void OnDecode(ByteBuffer buffer, int count)
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

        internal override int OnValueSize()
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
