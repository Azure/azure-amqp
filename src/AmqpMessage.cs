// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    [Flags]
    public enum SectionFlag
    {
        Header = 1,
        DeliveryAnnotations = 2,
        MessageAnnotations = 4,
        Properties = 8,
        ApplicationProperties = 16,
        Data = 32,
        AmqpSequence = 64,
        AmqpValue = 128,
        Footer = 256,
        All = Mutable | Immutable,
        Body = Data | AmqpSequence | AmqpValue,
        NonBody = All & ~Body,
        Mutable = Header | DeliveryAnnotations | MessageAnnotations | Footer,
        Immutable = Properties | ApplicationProperties | Body,
    }

    /// <summary>
    /// Implements the AMQP MESSAGE FORMAT 0 message.
    /// </summary>
    public abstract class AmqpMessage : Delivery
    {
        const int MessageDisposed = 1;
        const int BufferDisposed = 2;

        Header header;
        DeliveryAnnotations deliveryAnnotations;
        MessageAnnotations messageAnnotations;
        Properties properties;
        ApplicationProperties applicationProperties;
        Footer footer;

        // one of the following is allowed
        IList<Data> dataList;
        IList<AmqpSequence> sequenceList;
        AmqpValue amqpValue;

        SectionFlag sectionFlags;
        int messageSize = -1;
        int bodyOffset = -1;
        int bodyLength = 0;
        ByteBuffer buffer;
        int disposeState;

        public Header Header
        {
            get
            {
                this.EnsureInitialized<Header>(ref this.header, SectionFlag.Header);
                return this.header;
            }

            protected set
            {
                this.header = value;
                this.UpdateSectionFlag(value != null, SectionFlag.Header);
            }
        }

        public DeliveryAnnotations DeliveryAnnotations
        {
            get
            {
                this.EnsureInitialized<DeliveryAnnotations>(ref this.deliveryAnnotations, SectionFlag.DeliveryAnnotations);
                return this.deliveryAnnotations;
            }

            protected set
            {
                this.deliveryAnnotations = value;
                this.UpdateSectionFlag(value != null, SectionFlag.DeliveryAnnotations);
            }
        }

        public MessageAnnotations MessageAnnotations
        {
            get
            {
                this.EnsureInitialized<MessageAnnotations>(ref this.messageAnnotations, SectionFlag.MessageAnnotations);
                return this.messageAnnotations;
            }

            protected set
            {
                this.messageAnnotations = value;
                this.UpdateSectionFlag(value != null, SectionFlag.MessageAnnotations);
            }
        }

        public Properties Properties
        {
            get
            {
                this.EnsureInitialized<Properties>(ref this.properties, SectionFlag.Properties);
                return this.properties;
            }

            protected set
            {
                this.properties = value;
                this.UpdateSectionFlag(value != null, SectionFlag.Properties);
            }
        }

        public ApplicationProperties ApplicationProperties
        {
            get
            {
                this.EnsureInitialized<ApplicationProperties>(ref this.applicationProperties, SectionFlag.ApplicationProperties);
                return this.applicationProperties;
            }

            protected set
            {
                this.applicationProperties = value;
                this.UpdateSectionFlag(value != null, SectionFlag.ApplicationProperties);
            }
        }

        public IList<Data> DataBody
        {
            get
            {
                this.EnsureBodyType(SectionFlag.Data);
                return this.dataList;
            }
        }

        public IList<AmqpSequence> SequenceBody
        {
            get
            {
                this.EnsureBodyType(SectionFlag.AmqpSequence);
                return this.sequenceList;
            }
        }

        public AmqpValue ValueBody
        {
            get
            {
                this.EnsureBodyType(SectionFlag.AmqpValue);
                return this.amqpValue;
            }
        }

        public virtual Stream BodyStream
        {
            get
            {
                if (this.BodyOffset < 0 || this.buffer == null)
                {
                    return null;
                }

                if (this.dataList != null)
                {
                    return new BufferListStream(this.dataList.Select(d => (ArraySegment<byte>)d.Value).ToArray());
                }

                return new MemoryStream(this.buffer.Buffer, this.bodyOffset, this.BodyLength);
            }
        }

        public Footer Footer
        {
            get
            {
                this.EnsureInitialized<Footer>(ref this.footer, SectionFlag.Footer);
                return this.footer;
            }

            protected set
            {
                this.footer = value;
                this.UpdateSectionFlag(value != null, SectionFlag.Footer);
            }
        }

        public SectionFlag Sections
        {
            get
            {
                if (this.sectionFlags == 0 && this.buffer != null)
                {
                    this.Initialize(SectionFlag.All);
                }

                return this.sectionFlags;
            }
        }

        public SectionFlag BodyType
        {
            get
            {
                if (this.sectionFlags == 0 && this.buffer != null)
                {
                    this.Initialize(SectionFlag.All);
                }

                return this.sectionFlags & SectionFlag.Body;
            }
        }

        protected ByteBuffer Buffer
        {
            get { return this.buffer; }
            set { this.buffer = value; }
        }

        internal int BodyOffset
        {
            get
            {
                this.Initialize(SectionFlag.All);
                return this.bodyOffset;
            }
        }

        internal int BodyLength
        {
            get
            {
                this.Initialize(SectionFlag.All);
                return this.bodyLength;
            }
        }

        // Public factory methods
        public static AmqpMessage Create()
        {
            return new AmqpEmptyBodyMessage();
        }

        public static AmqpMessage Create(Data data)
        {
            return Create(new Data[] { data });
        }

        public static AmqpMessage Create(IList<Data> dataList)
        {
            return new AmqpDataMessage(dataList);
        }

        public static AmqpMessage Create(AmqpValue value)
        {
            return new AmqpValueMessage(value);
        }

        public static AmqpMessage Create(IList<AmqpSequence> amqpSequence)
        {
            return new AmqpSequenceMessage(amqpSequence);
        }

        public static AmqpMessage Create(Stream stream, bool ownStream)
        {
            return new AmqpBodyStreamMessage(stream, ownStream);
        }

        // Internal factory methods
        public static AmqpMessage CreateReceivedMessage()
        {
            return new AmqpBufferMessage();
        }

        public static AmqpMessage CreateBufferMessage(ByteBuffer buffer)
        {
            return new AmqpBufferMessage(buffer);
        }

        public long Serialize(bool force)
        {
            this.Initialize(SectionFlag.All, force);
            return this.messageSize;
        }

        public AmqpMessage Clone()
        {
            return this.Clone(false);
        }

        public AmqpMessage Clone(bool deepCopy)
        {
            if (this.Link != null && !this.Link.IsReceiver)
            {
                throw new InvalidOperationException(Resources.AmqpCannotCloneSentMessage);
            }

            if (this.sectionFlags == 0)
            {
                // a received message that has not been deserialized.
                this.Initialize(SectionFlag.All);
            }

            return new AmqpClonedMessage(this, deepCopy);
        }

        public override void AddPayload(ByteBuffer payload, bool isLast)
        {
            throw new InvalidOperationException();
        }

        public override ByteBuffer GetPayload(int payloadSize, out bool more)
        {
            this.Initialize(SectionFlag.All);
            return GetPayload(this.buffer, payloadSize, out more);
        }

        public override void Dispose()
        {
            this.disposeState |= MessageDisposed;
            this.ReleaseBuffer();
        }

        public void ThrowIfDisposed()
        {
            if ((this.disposeState & MessageDisposed) > 0)
            {
                throw new ObjectDisposedException(this.GetType().Name);
            }
        }

        protected static void EncodeSection(ByteBuffer buffer, AmqpDescribed section)
        {
            if (section != null)
            {
                section.Offset = buffer.WritePos;
                section.Encode(buffer);
                section.Length = buffer.WritePos - section.Offset;
            }
        }

        protected abstract void Initialize(SectionFlag desiredSections, bool force = false);

        protected virtual void EnsureInitialized<T>(ref T obj, SectionFlag section) where T : class, new()
        {
            if (AmqpMessage.EnsureInitialized(ref obj))
            {
                this.sectionFlags |= section;
            }
        }

        void ReleaseBuffer()
        {
            // if other bits are set after unset is computed, the buffer may not be disposed
            // this is ok as long as the buffer is not double disposed.
            int unset = this.disposeState & ~BufferDisposed;
            int set = this.disposeState | BufferDisposed;
            int original = Interlocked.CompareExchange(ref this.disposeState, set, unset);
            if ((original & BufferDisposed) == 0)
            {
                this.buffer?.Dispose();
            }
        }

        static bool EnsureInitialized<T>(ref T obj) where T : class, new()
        {
            if (obj == null)
            {
                obj = new T();
                return true;
            }
            else
            {
                return false;
            }
        }

        void CopyFrom(AmqpMessage source, bool deepCopy, bool includeBody)
        {
            if (deepCopy)
            {
                this.DeepCopy(source);
            }
            else
            {
                this.ShadowCopy(source);
            }

            if (includeBody)
            {
                this.dataList = source.dataList;
                this.sequenceList = source.sequenceList;
                this.amqpValue = source.amqpValue;
                this.bodyOffset = source.bodyOffset;
                this.bodyLength = source.bodyLength;
                this.sectionFlags |= (source.sectionFlags & SectionFlag.Body);
            }
        }

        void ShadowCopy(AmqpMessage source)
        {
            this.header = source.header;
            this.deliveryAnnotations = source.deliveryAnnotations;
            this.messageAnnotations = source.messageAnnotations;
            this.properties = source.properties;
            this.applicationProperties = source.applicationProperties;
            this.footer = source.footer;
            this.sectionFlags |= (source.sectionFlags & SectionFlag.NonBody);
        }

        void DeepCopy(AmqpMessage source)
        {
            if (source.header != null)
            {
                this.header = new Header();
                this.header.Durable = source.header.Durable;
                this.header.Priority = source.header.Priority;
                this.header.Ttl = source.header.Ttl;
                this.header.FirstAcquirer = source.header.FirstAcquirer;
                this.header.DeliveryCount = source.header.DeliveryCount;
            }

            if (source.deliveryAnnotations != null)
            {
                this.deliveryAnnotations = new DeliveryAnnotations();
                this.deliveryAnnotations.Map.Merge(source.deliveryAnnotations.Map);
            }

            if (source.messageAnnotations != null)
            {
                this.messageAnnotations = new MessageAnnotations();
                this.messageAnnotations.Map.Merge(source.messageAnnotations.Map);
            }

            if (source.applicationProperties != null)
            {
                this.applicationProperties = new ApplicationProperties();
                this.applicationProperties.Map.Merge(source.applicationProperties.Map);
            }

            if (source.properties != null)
            {
                this.properties = new Properties();
                this.properties.MessageId = source.properties.MessageId;
                this.properties.UserId = source.properties.UserId;
                this.properties.To = source.properties.To;
                this.properties.Subject = source.properties.Subject;
                this.properties.ReplyTo = source.properties.ReplyTo;
                this.properties.CorrelationId = source.properties.CorrelationId;
                this.properties.ContentType = source.properties.ContentType;
                this.properties.ContentEncoding = source.properties.ContentEncoding;
                this.properties.AbsoluteExpiryTime = source.properties.AbsoluteExpiryTime;
                this.properties.CreationTime = source.properties.CreationTime;
                this.properties.GroupId = source.properties.GroupId;
                this.properties.GroupSequence = source.properties.GroupSequence;
                this.properties.ReplyToGroupId = source.properties.ReplyToGroupId;
            }

            if (source.footer != null)
            {
                this.footer = new Footer();
                this.footer.Map.Merge(source.footer.Map);
            }

            this.sectionFlags |= (source.sectionFlags & SectionFlag.NonBody);
        }

        void EnsureBodyType(SectionFlag expected)
        {
            if (this.sectionFlags == 0 && this.buffer != null)
            {
                this.Initialize(SectionFlag.All);
            }

            SectionFlag actual = this.sectionFlags & SectionFlag.Body;
            if (actual > 0 && actual != expected)
            {
                throw new InvalidOperationException(AmqpResources.GetString(Resources.AmqpInvalidMessageBodyType, actual, expected));
            }
        }

        void UpdateSectionFlag(bool set, SectionFlag flag)
        {
            if (set)
            {
                this.sectionFlags |= flag;
            }
            else
            {
                this.sectionFlags &= ~flag;
            }
        }

        /// <summary>
        /// Message is composed of different sections. Sections are settable/mutable.
        /// </summary>
        abstract class AmqpSectionMessage : AmqpMessage
        {
            protected override void Initialize(SectionFlag desiredSections, bool force = false)
            {
                if (force || this.buffer == null)
                {
                    this.buffer?.Dispose();
                    this.buffer = this.Encode();
                    this.messageSize = this.buffer.Length;
                }
            }

            public override void CompletePayload(int payloadSize)
            {
                Fx.Assert(this.buffer != null, "buffer not initialized");
                base.CompletePayload(payloadSize);
                this.buffer.Complete(payloadSize);
                if (this.buffer.Length == 0)
                {
                    this.ReleaseBuffer();
                }
            }

            protected virtual void EncodeBody(ByteBuffer buffer)
            {
                int pos = buffer.WritePos;
                if ((this.sectionFlags & SectionFlag.Data) != 0)
                {
                    foreach (Data data in this.dataList)
                    {
                        EncodeSection(buffer, data);
                    }
                }
                else if ((this.sectionFlags & SectionFlag.AmqpValue) != 0)
                {
                    EncodeSection(buffer, this.amqpValue);
                }
                else if ((this.sectionFlags & SectionFlag.AmqpSequence) != 0)
                {
                    foreach (AmqpSequence seq in this.sequenceList)
                    {
                        EncodeSection(buffer, seq);
                    }
                }

                if (buffer.WritePos > pos)
                {
                    this.bodyOffset = pos;
                    this.bodyLength = buffer.WritePos - pos;
                }
            }

            ByteBuffer Encode()
            {
                ByteBuffer buffer = new ByteBuffer(1024, true);
                EncodeSection(buffer, this.header);
                EncodeSection(buffer, this.deliveryAnnotations);
                EncodeSection(buffer, this.messageAnnotations);
                EncodeSection(buffer, this.properties);
                EncodeSection(buffer, this.applicationProperties);
                this.EncodeBody(buffer);
                EncodeSection(buffer, this.footer);
                return buffer;
            }
        }

        sealed class AmqpEmptyBodyMessage : AmqpSectionMessage
        {
        }

        sealed class AmqpValueMessage : AmqpSectionMessage
        {
            public AmqpValueMessage(AmqpValue value)
            {
                this.amqpValue = value;
                this.sectionFlags |= SectionFlag.AmqpValue;
            }
        }

        sealed class AmqpDataMessage : AmqpSectionMessage
        {
            public AmqpDataMessage(IList<Data> dataList)
            {
                this.dataList = dataList;
                this.sectionFlags |= SectionFlag.Data;
            }
        }

        sealed class AmqpSequenceMessage : AmqpSectionMessage
        {
            public AmqpSequenceMessage(IList<AmqpSequence> sequence)
            {
                this.sequenceList = sequence;
                this.sectionFlags |= SectionFlag.AmqpSequence;
            }
        }

        /// <summary>
        /// Allow updates in mutalble sections.
        /// Note: currently this is only used by relay. 
        /// </summary>
        sealed class AmqpClonedMessage : AmqpSectionMessage
        {
            readonly bool deepCopy;
            readonly ByteBuffer source;

            public AmqpClonedMessage(AmqpMessage source, bool deepCopy)
            {
                this.deepCopy = deepCopy;
                this.CopyFrom(source, deepCopy, true);
                this.source = source.buffer?.AddReference();
            }

            protected override void Initialize(SectionFlag desiredSections, bool force = false)
            {
                if (this.buffer != null && !force)
                {
                    return;
                }

                int size = this.source == null ? 1024 : this.source.Length;
                ByteBuffer buffer = new ByteBuffer(size, true);
                AmqpMessage.EncodeSection(buffer, this.header);
                AmqpMessage.EncodeSection(buffer, this.deliveryAnnotations);
                AmqpMessage.EncodeSection(buffer, this.messageAnnotations);
                if (this.deepCopy)
                {
                    AmqpMessage.EncodeSection(buffer, this.properties);
                    AmqpMessage.EncodeSection(buffer, this.applicationProperties);
                }
                else
                {
                    WriteSection(buffer, this.properties, this.source);
                    WriteSection(buffer, this.applicationProperties, this.source);
                }

                if (this.source != null && this.bodyOffset >= 0)
                {
                    AmqpBitConverter.WriteBytes(buffer, this.source.Buffer, this.bodyOffset, this.bodyLength);
                }
                else
                {
                    this.EncodeBody(buffer);
                }

                AmqpMessage.EncodeSection(buffer, this.footer);
                this.buffer?.Dispose();
                this.buffer = buffer;
                this.messageSize = buffer.Length;
            }

            public override void Dispose()
            {
                this.source?.Dispose();
                base.Dispose();
            }

            static void WriteSection(ByteBuffer buffer, AmqpDescribed section, ByteBuffer source)
            {
                if (section != null)
                {
                    if (source != null)
                    {
                        AmqpBitConverter.WriteBytes(buffer, source.Buffer, section.Offset, section.Length);
                    }
                    else
                    {
                        AmqpMessage.EncodeSection(buffer, section);
                    }
                }
            }
        }

        /// <summary>
        /// Wraps a stream in the message body. The data is sent in one or more Data sections.
        /// </summary>
        sealed class AmqpBodyStreamMessage : AmqpSectionMessage
        {
            readonly Stream bodyStream;
            readonly bool ownStream;

            public AmqpBodyStreamMessage(Stream bodyStream, bool ownStream)
            {
                this.sectionFlags |= SectionFlag.Data;
                this.bodyStream = bodyStream;
                this.ownStream = ownStream;
            }

            public override Stream BodyStream
            {
                get
                {
                    return this.bodyStream;
                }
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
                int pos = buffer.WritePos;
                buffer.Validate(true, 8);
                buffer.Append(8);
                int length = 0;
                long streamPos = this.bodyStream.CanSeek ? this.bodyStream.Position : -1;
                try
                {
                    while (true)
                    {
                        buffer.Validate(true, 512);
                        int size = this.bodyStream.Read(buffer.Buffer, buffer.WritePos, 512);
                        if (size == 0)
                        {
                            break;
                        }

                        buffer.Append(size);
                        length += size;
                    }
                }
                finally
                {
                    if (this.bodyStream.CanSeek)
                    {
                        this.bodyStream.Position = streamPos;
                    }
                }

                AmqpBitConverter.WriteUByte(buffer.Buffer, pos, FormatCode.Described);
                AmqpBitConverter.WriteUByte(buffer.Buffer, pos + 1, FormatCode.SmallULong);
                AmqpBitConverter.WriteUByte(buffer.Buffer, pos + 2, (byte)Data.Code);
                AmqpBitConverter.WriteUByte(buffer.Buffer, pos + 3, FormatCode.Binary32);
                AmqpBitConverter.WriteUInt(buffer.Buffer, pos + 4, (uint)length);

                this.bodyOffset = pos;
                this.bodyLength = buffer.WritePos - pos;
                this.dataList = new Data[]
                {
                    new Data()
                    {
                        Value = new ArraySegment<byte>(buffer.Buffer, pos, length),
                        Offset = pos,
                        Length = length
                    }
                };
            }

            public override void Dispose()
            {
                base.Dispose();
                if (this.ownStream)
                {
                    this.bodyStream.Dispose();
                }
            }
        }

        /// <summary>
        /// Message is constructed from a buffer. Sections are not settable. Though
        /// not strictly enforced, section properties SHOULD NOT be updated as the
        /// change will be lost. To update a buffer message, clone it first and update
        /// the cloned message instead.
        /// </summary>
        sealed class AmqpBufferMessage : AmqpMessage
        {
            public AmqpBufferMessage()
            {
            }

            public AmqpBufferMessage(ByteBuffer buffer)
            {
                this.buffer = buffer;
                this.messageSize = buffer.Length;
            }

            public override void AddPayload(ByteBuffer payload, bool isLast)
            {
                this.BytesTransfered += payload.Length;
                this.buffer = AddPayload(this.buffer, payload, isLast);
                this.messageSize = this.buffer.Length;
            }

            public override ByteBuffer GetPayload(int payloadSize, out bool more)
            {
                return GetPayload(this.buffer, payloadSize, out more);
            }

            protected override void Initialize(SectionFlag desiredSections, bool force = false)
            {
                if (this.sectionFlags > 0)
                {
                    return;
                }

                Error error;
                if (!AmqpMessageReader.TryRead(
                    this,
                    0,
                    this.buffer,
                    desiredSections,
                    (a, b, s) => HandleSection(a, b, s),
                    this.GetType().Name,
                    out error))
                {
                    throw new AmqpException(error);
                }
            }

            protected override void EnsureInitialized<T>(ref T obj, SectionFlag section)
            {
                this.Initialize(SectionFlag.All);
            }

            static bool HandleSection(AmqpBufferMessage thisPtr, int unused, Section section)
            {
                if (section.Flag == SectionFlag.Header)
                {
                    thisPtr.Header = (Header)section.Value;
                }
                else if (section.Flag == SectionFlag.DeliveryAnnotations)
                {
                    thisPtr.DeliveryAnnotations = (DeliveryAnnotations)section.Value;
                }
                else if (section.Flag == SectionFlag.MessageAnnotations)
                {
                    thisPtr.MessageAnnotations = (MessageAnnotations)section.Value;
                }
                else if (section.Flag == SectionFlag.Properties)
                {
                    thisPtr.Properties = (Properties)section.Value;
                }
                else if (section.Flag == SectionFlag.ApplicationProperties)
                {
                    thisPtr.ApplicationProperties = (ApplicationProperties)section.Value;
                }
                else if (section.Flag == SectionFlag.Data)
                {
                    if (thisPtr.dataList == null)
                    {
                        thisPtr.bodyOffset = section.Offset;
                        thisPtr.dataList = new List<Data>();
                        thisPtr.sectionFlags |= SectionFlag.Data;
                    }

                    thisPtr.bodyLength += section.Length;
                    thisPtr.dataList.Add((Data)section.Value);
                }
                else if (section.Flag == SectionFlag.AmqpSequence)
                {
                    if (thisPtr.sequenceList == null)
                    {
                        thisPtr.bodyOffset = section.Offset;
                        thisPtr.sequenceList = new List<AmqpSequence>();
                        thisPtr.sectionFlags |= SectionFlag.AmqpSequence;
                    }

                    thisPtr.bodyLength += section.Length;
                    thisPtr.sequenceList.Add((AmqpSequence)section.Value);
                }
                else if (section.Flag == SectionFlag.AmqpValue)
                {
                    thisPtr.bodyOffset = section.Offset;
                    thisPtr.bodyLength = section.Length;
                    thisPtr.amqpValue = (AmqpValue)section.Value;
                    thisPtr.sectionFlags |= SectionFlag.AmqpValue;
                }
                else if (section.Flag == SectionFlag.Footer)
                {
                    thisPtr.Footer = (Footer)section.Value;
                }
                else
                {
                    throw new AmqpException(AmqpErrorCode.DecodeError,
                        AmqpResources.GetString(Resources.AmqpInvalidMessageSectionCode, section.Flag));
                }

                return true;
            }
        }
    }
}
