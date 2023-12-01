// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Globalization;
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class MessageId
    {
        public abstract int EncodeSize { get; }

        public static implicit operator MessageId(ulong value)
        {
            return new MessageIdUlong(value);
        }

        public static implicit operator MessageId(Guid value)
        {
            return new MessageIdUuid(value);
        }

        public static implicit operator MessageId(ArraySegment<byte> value)
        {
            return new MessageIdBinary(value);
        }

        public static implicit operator MessageId(string value)
        {
            return new MessageIdString(value);
        }

        public static int GetEncodeSize(MessageId messageId)
        {
            return messageId == null ? FixedWidth.NullEncoded : messageId.EncodeSize;
        }

        public static void Encode(ByteBuffer buffer, MessageId messageId)
        {
            if (messageId == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                messageId.OnEncode(buffer);
            }
        }

        public static MessageId Decode(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            switch (formatCode)
            {
                case FormatCode.Null:
                    return null;
                case FormatCode.String8Utf8:
                case FormatCode.String32Utf8:
                    string str = StringEncoding.Decode(buffer, formatCode);
                    return new MessageIdString(str);
                case FormatCode.ULong0:
                    return new MessageIdUlong(0ul);
                case FormatCode.SmallULong:
                case FormatCode.ULong:
                    ulong? value = ULongEncoding.Decode(buffer, formatCode);
                    return new MessageIdUlong(value.Value);
                case FormatCode.Uuid:
                    Guid? uuid = UuidEncoding.Decode(buffer, formatCode);
                    return new MessageIdUuid(uuid.Value);
                case FormatCode.Binary8:
                case FormatCode.Binary32:
                    ArraySegment<byte> bin = BinaryEncoding.Decode(buffer, formatCode, false);
                    return new MessageIdBinary(bin);
                default:
                    throw new AmqpException(AmqpErrorCode.InvalidField, $"Format code {formatCode} is not valid for a message ID type.");
            }
        }

        public abstract void OnEncode(ByteBuffer buffer);

        sealed class MessageIdUlong : MessageId
        {
            ulong id;

            public MessageIdUlong(ulong id)
            {
                this.id = id;
            }

            public override int EncodeSize
            {
                get { return AmqpCodec.GetULongEncodeSize(this.id); }
            }

            public override void OnEncode(ByteBuffer buffer)
            {
                AmqpCodec.EncodeULong(this.id, buffer);
            }

            public override bool Equals(object obj)
            {
                MessageIdUlong other = obj as MessageIdUlong;
                if (other == null)
                {
                    return false;
                }

                return this.id == other.id;
            }

            public override string ToString()
            {
                return this.id.ToString(CultureInfo.InvariantCulture);
            }

            public override int GetHashCode()
            {
                return this.id.GetHashCode();
            }
        }

        sealed class MessageIdUuid : MessageId
        {
            Guid id;

            public MessageIdUuid(Guid id)
            {
                this.id = id;
            }

            public override int EncodeSize
            {
                get { return AmqpCodec.GetUuidEncodeSize(this.id); }
            }

            public override void OnEncode(ByteBuffer buffer)
            {
                AmqpCodec.EncodeUuid(this.id, buffer);
            }

            public override bool Equals(object obj)
            {
                MessageIdUuid other = obj as MessageIdUuid;
                if (other == null)
                {
                    return false;
                }

                return this.id == other.id;
            }

            public override string ToString()
            {
                return this.id.ToString();
            }

            public override int GetHashCode()
            {
                return this.id.GetHashCode();
            }
        }

        sealed class MessageIdBinary : MessageId
        {
            ArraySegment<byte> id;

            public MessageIdBinary(ArraySegment<byte> id)
            {
                this.id = id;
            }

            public override int EncodeSize
            {
                get { return AmqpCodec.GetBinaryEncodeSize(this.id); }
            }

            public override void OnEncode(ByteBuffer buffer)
            {
                AmqpCodec.EncodeBinary(this.id, buffer);
            }

            public override bool Equals(object obj)
            {
                MessageIdBinary other = obj as MessageIdBinary;
                if (other == null)
                {
                    return false;
                }

                return ByteArrayComparer.Instance.Equals(this.id, other.id);
            }

            public override int GetHashCode()
            {
                return ByteArrayComparer.Instance.GetHashCode(this.id);
            }

            public override string ToString()
            {
                return Extensions.GetString(this.id);
            }
        }

        sealed class MessageIdString : MessageId
        {
            string id;

            public MessageIdString(string id)
            {
                this.id = id;
            }

            public override int EncodeSize
            {
                get { return AmqpCodec.GetStringEncodeSize(this.id); }
            }

            public override void OnEncode(ByteBuffer buffer)
            {
                AmqpCodec.EncodeString(this.id, buffer);
            }

            public override bool Equals(object obj)
            {
                MessageIdString other = obj as MessageIdString;
                if (other == null)
                {
                    return false;
                }

                return this.id.Equals(other.id);
            }

            public override string ToString()
            {
                return this.id;
            }

            public override int GetHashCode()
            {
                return this.id.GetHashCode();
            }
        }
    }
}
