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

    /// <summary>
    /// Flags for message sections.
    /// </summary>
    [Flags]
    public enum SectionFlag
    {
        /// <summary>
        /// Message header.
        /// </summary>
        Header = 1,
        /// <summary>
        /// Delivery annotations.
        /// </summary>
        DeliveryAnnotations = 2,
        /// <summary>
        /// Message annotations.
        /// </summary>
        MessageAnnotations = 4,
        /// <summary>
        /// Properties.
        /// </summary>
        Properties = 8,
        /// <summary>
        /// Application Properties.
        /// </summary>
        ApplicationProperties = 16,
        /// <summary>
        /// Data in the body.
        /// </summary>
        Data = 32,
        /// <summary>
        /// AmqpSequence in the body.
        /// </summary>
        AmqpSequence = 64,
        /// <summary>
        /// AmqpValue in the body.
        /// </summary>
        AmqpValue = 128,
        /// <summary>
        /// Footer.
        /// </summary>
        Footer = 256,
        /// <summary>
        /// All sections.
        /// </summary>
        All = Mutable | Immutable,
        /// <summary>
        /// Body section.
        /// </summary>
        Body = Data | AmqpSequence | AmqpValue,
        /// <summary>
        /// Non-body sections.
        /// </summary>
        NonBody = All & ~Body,
        /// <summary>
        /// Mutable sections.
        /// </summary>
        Mutable = Header | DeliveryAnnotations | MessageAnnotations | Footer,
        /// <summary>
        /// Immutable sections.
        /// </summary>
        Immutable = Properties | ApplicationProperties | Body,
    }

    /// <summary>
    /// Implements the AMQP message.
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

        /// <summary>
        /// Gets or sets the header.
        /// </summary>
        public Header Header
        {
            get
            {
                this.EnsureInitialized<Header>(ref this.header, SectionFlag.Header);
                return this.header;
            }

            set
            {
                this.header = value;
                this.UpdateSectionFlag(value != null, SectionFlag.Header);
            }
        }

        /// <summary>
        /// Gets or sets the delivery annotations.
        /// </summary>
        public DeliveryAnnotations DeliveryAnnotations
        {
            get
            {
                this.EnsureInitialized<DeliveryAnnotations>(ref this.deliveryAnnotations, SectionFlag.DeliveryAnnotations);
                return this.deliveryAnnotations;
            }

            set
            {
                this.deliveryAnnotations = value;
                this.UpdateSectionFlag(value != null, SectionFlag.DeliveryAnnotations);
            }
        }

        /// <summary>
        /// Gets or sets the message annotations.
        /// </summary>
        public MessageAnnotations MessageAnnotations
        {
            get
            {
                this.EnsureInitialized<MessageAnnotations>(ref this.messageAnnotations, SectionFlag.MessageAnnotations);
                return this.messageAnnotations;
            }

            set
            {
                this.messageAnnotations = value;
                this.UpdateSectionFlag(value != null, SectionFlag.MessageAnnotations);
            }
        }

        /// <summary>
        /// Gets or sets the properties.
        /// </summary>
        public Properties Properties
        {
            get
            {
                this.EnsureInitialized<Properties>(ref this.properties, SectionFlag.Properties);
                return this.properties;
            }

            set
            {
                this.properties = value;
                this.UpdateSectionFlag(value != null, SectionFlag.Properties);
            }
        }

        /// <summary>
        /// Gets or sets the application properties.
        /// </summary>
        public ApplicationProperties ApplicationProperties
        {
            get
            {
                this.EnsureInitialized<ApplicationProperties>(ref this.applicationProperties, SectionFlag.ApplicationProperties);
                return this.applicationProperties;
            }

            set
            {
                this.applicationProperties = value;
                this.UpdateSectionFlag(value != null, SectionFlag.ApplicationProperties);
            }
        }

        /// <summary>
        /// Gets the Data list body.
        /// </summary>
        public IList<Data> DataBody
        {
            get
            {
                this.EnsureBodyType(SectionFlag.Data);
                return this.dataList;
            }
        }

        /// <summary>
        /// Gets the AmqpSequence body.
        /// </summary>
        public IList<AmqpSequence> SequenceBody
        {
            get
            {
                this.EnsureBodyType(SectionFlag.AmqpSequence);
                return this.sequenceList;
            }
        }

        /// <summary>
        /// Gets the AmqpValue body.
        /// </summary>
        public AmqpValue ValueBody
        {
            get
            {
                this.EnsureBodyType(SectionFlag.AmqpValue);
                return this.amqpValue;
            }
        }

        /// <summary>
        /// Gets the body stream.
        /// </summary>
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

        /// <summary>
        /// Gets the footer.
        /// </summary>
        public Footer Footer
        {
            get
            {
                this.EnsureInitialized<Footer>(ref this.footer, SectionFlag.Footer);
                return this.footer;
            }

            set
            {
                this.footer = value;
                this.UpdateSectionFlag(value != null, SectionFlag.Footer);
            }
        }

        /// <summary>
        /// Gets the section flags of the message.
        /// </summary>
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

        /// <summary>
        /// Gets the body flag.
        /// </summary>
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

        /// <summary>
        /// Gets the number of bytes of the serialized message.
        /// </summary>
        public long SerializedMessageSize
        {
            get => this.Serialize(true);
        }

        /// <summary>
        /// Gets or sets a buffer of the message.
        /// </summary>
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

        /// <summary>
        /// Creates a message without the body.
        /// </summary>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage Create()
        {
            return new AmqpEmptyBodyMessage();
        }

        /// <summary>
        /// Creates a message with a Data as the body.
        /// </summary>
        /// <param name="data">The body.</param>
        /// <returns></returns>
        public static AmqpMessage Create(Data data)
        {
            return Create(new Data[] { data });
        }

        /// <summary>
        /// Creates a message with a Data list as the body.
        /// </summary>
        /// <param name="dataList"></param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage Create(IList<Data> dataList)
        {
            return new AmqpDataMessage(dataList);
        }

        /// <summary>
        /// Creates a message with an AMQP object in the <see cref="AmqpValue"/> body.
        /// </summary>
        /// <param name="value">The AMQP object.</param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage Create(object value)
        {
            return new AmqpValueMessage(new AmqpValue() { Value = value });
        }

        /// <summary>
        /// Creates a message with an AmqpValue as the body.
        /// </summary>
        /// <param name="value"></param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage Create(AmqpValue value)
        {
            return new AmqpValueMessage(value);
        }

        /// <summary>
        /// Creates a message with a list of AmqpSequence as the body.
        /// </summary>
        /// <param name="amqpSequence"></param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage Create(IList<AmqpSequence> amqpSequence)
        {
            return new AmqpSequenceMessage(amqpSequence);
        }

        /// <summary>
        /// Creates a message with a stream as the body. Bytes from the stream
        /// is put in a Data section.
        /// </summary>
        /// <param name="stream">The body stream.</param>
        /// <param name="ownStream">True if the message should own the stream.</param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage Create(Stream stream, bool ownStream)
        {
            return new AmqpBodyStreamMessage(stream, ownStream);
        }

        /// <summary>
        /// Creates a message whose buffer will be updated later when transfers are
        /// received.
        /// </summary>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage CreateReceivedMessage()
        {
            return new AmqpBufferMessage();
        }

        /// <summary>
        /// Creates a message from a buffer. Message is deserialized when it is accessed.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage CreateBufferMessage(ByteBuffer buffer)
        {
            return new AmqpBufferMessage(buffer);
        }

        /// <summary>
        /// Creates a message from a stream.
        /// </summary>
        /// <param name="messageStream">The message stream.</param>
        /// <param name="ownStream">true if the stream will be owned by the message.</param>
        /// <returns>An AmqpMessage.</returns>
        public static AmqpMessage CreateAmqpStreamMessage(BufferListStream messageStream, bool ownStream = true)
        {
            ArraySegment<byte> payload = messageStream.ReadBytes(int.MaxValue);
            return new AmqpBufferMessage(new ByteBuffer(payload));
        }

        /// <summary>
        /// Serializes the message and update its <see cref="Buffer"/>.
        /// </summary>
        /// <param name="force">True to force update the buffer if it exists.</param>
        /// <returns>An AmqpMessage.</returns>
        public long Serialize(bool force)
        {
            this.Initialize(SectionFlag.All, force);
            return this.messageSize;
        }

        /// <summary>
        /// Creates a stream from the bytes of the serialized message.
        /// </summary>
        /// <returns></returns>
        public Stream ToStream()
        {
            this.Initialize(SectionFlag.All, true);
            return new MemoryStream(this.buffer.Buffer, this.buffer.Offset, this.buffer.Length);
        }

        /// <summary>
        /// Clones the message for send or other operations.
        /// </summary>
        /// <returns>An AmqpMessage.</returns>
        /// <remarks>
        /// Mutable sections can be added or updated. This method performs
        /// a shadow copy of the sections so any update is also affecting the
        /// source message.
        /// </remarks>
        public AmqpMessage Clone()
        {
            return this.Clone(false);
        }

        /// <summary>
        /// Clones the message for send or other operations.
        /// </summary>
        /// <param name="deepCopy">True to perform a deep copy of all multable sections.</param>
        /// <returns>An AmqpMessage.</returns>
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

        /// <summary>
        /// Appends a buffer to the message.
        /// </summary>
        /// <param name="payload">The payload buffer.</param>
        /// <param name="isLast">True if it is the last part of the message.</param>
        public override void AddPayload(ByteBuffer payload, bool isLast)
        {
            throw new InvalidOperationException();
        }

        /// <summary>
        /// Gets a buffer of the message payload from the current read position and advances
        /// the read position.
        /// </summary>
        /// <param name="payloadSize">The size of the buffer to return.</param>
        /// <param name="more">True if there is no more data in the message's buffer.</param>
        /// <returns>A buffer of the requested payload. It can be smaller than the requested size.</returns>
        public override ByteBuffer GetPayload(int payloadSize, out bool more)
        {
            this.Initialize(SectionFlag.All);
            return GetPayload(this.buffer, payloadSize, out more);
        }

        /// <summary>
        /// Gets the payload segments of the serialized message. It should not be used.
        /// Use <see cref="GetPayload(int, out bool)"/> instead.
        /// </summary>
        /// <returns>The serialized message bytes in an array of byte segments.</returns>
        public ArraySegment<byte>[] GetPayload()
        {
            var buffer = this.GetPayload(int.MaxValue, out _);
            return buffer == null ? null : new ArraySegment<byte>[] { buffer.AsSegment() };
        }

        /// <summary>
        /// Disposes the message and releases the buffer.
        /// </summary>
        public override void Dispose()
        {
            this.disposeState |= MessageDisposed;
            this.ReleaseBuffer();
        }

        internal void ThrowIfDisposed()
        {
            if ((this.disposeState & MessageDisposed) > 0)
            {
                throw new ObjectDisposedException(this.GetType().Name);
            }
        }

        internal static void EncodeSection(ByteBuffer buffer, AmqpDescribed section)
        {
            if (section != null)
            {
                section.Offset = buffer.WritePos;
                section.Encode(buffer);
                section.Length = buffer.WritePos - section.Offset;
            }
        }

        /// <summary>
        /// Initializes the message. Depends on the type of the message, it performs
        /// a serialization or deserialization of the message.
        /// </summary>
        /// <param name="desiredSections">The sections to initialize.</param>
        /// <param name="force">Force the initialization if it was done before.</param>
        protected abstract void Initialize(SectionFlag desiredSections, bool force = false);

        internal virtual void EnsureInitialized<T>(ref T obj, SectionFlag section) where T : class, new()
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
                buffer.ValidateWrite(8);
                buffer.Append(8);
                int length = 0;
                long streamPos = this.bodyStream.CanSeek ? this.bodyStream.Position : -1;
                try
                {
                    while (true)
                    {
                        buffer.ValidateWrite(512);
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

                buffer.Buffer[pos] = FormatCode.Described;
                buffer.Buffer[pos + 1] = FormatCode.SmallULong;
                buffer.Buffer[pos + 2] = (byte)Data.Code;
                buffer.Buffer[pos + 3] = FormatCode.Binary32;
                AmqpBitConverter.WriteUInt(buffer.Buffer, pos + 4, (uint)length);

                this.bodyOffset = pos;
                this.bodyLength = buffer.WritePos - pos;
                this.dataList = new Data[]
                {
                    new Data()
                    {
                        Segment = new ArraySegment<byte>(buffer.Buffer, pos, length),
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

            internal override void EnsureInitialized<T>(ref T obj, SectionFlag section)
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
