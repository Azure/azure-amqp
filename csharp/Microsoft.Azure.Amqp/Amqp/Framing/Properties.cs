// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Properties : DescribedList
    {
        public static readonly string Name = "amqp:properties:list";
        public static readonly ulong Code = 0x0000000000000073;
        static readonly string MessageIdName = "message-id";
        static readonly string UserIdName = "user-id";
        static readonly string ToName = "to";
        static readonly string SubjectName = "subject";
        static readonly string ReplyToName = "reply-to";
        static readonly string CorrelationIdName = "correlation-id";
        static readonly string ContentTypeName = "content-type";
        static readonly string ContentEncodingName = "content-encoding";
        static readonly string AbsoluteExpiryTimeName = "absolute-expiry-time";
        static readonly string CreationTimeName = "creation-time";
        static readonly string GroupIdName = "group-id";
        static readonly string GroupSequenceName = "group-sequence";
        static readonly string ReplyToGroupIdName = "reply-to-group-id";

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

        protected override int FieldCount
        {
            get { return Fields; }
        }

        public IDictionary<string, object> ToDictionary()
        {
            IDictionary<string, object> properties = new Dictionary<string, object>(StringComparer.OrdinalIgnoreCase);

            properties.Add(this.MessageId != null, MessageIdName, this.MessageId);
            properties.Add(this.UserId.Array != null, UserIdName, this.UserId);
            properties.Add(this.To != null, ToName, this.To);
            properties.Add(this.Subject != null, SubjectName, this.Subject);
            properties.Add(this.ReplyTo != null, ReplyToName, this.ReplyTo);
            properties.Add(this.CorrelationId != null, CorrelationIdName, this.CorrelationId);
            properties.Add(this.ContentType.Value != null, ContentTypeName, this.ContentType);
            properties.Add(this.ContentEncoding.Value != null, ContentEncodingName, this.ContentEncoding);
            properties.Add(this.AbsoluteExpiryTime != null, AbsoluteExpiryTimeName, this.AbsoluteExpiryTime);
            properties.Add(this.CreationTime != null, CreationTimeName, this.CreationTime);
            properties.Add(this.GroupId != null, GroupIdName, this.GroupId);
            properties.Add(this.GroupSequence != null, GroupSequenceName, this.GroupSequence);
            properties.Add(this.ReplyToGroupId != null, ReplyToGroupIdName, this.ReplyToGroupId);

            return properties;
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

    static class IDictionaryExtension
    {
        public static void Add(this IDictionary<string, object> dictionary, bool condition, string key, object value)
        {
            if (condition)
            {
                dictionary.Add(key, value);
            }
        }
    }
}
