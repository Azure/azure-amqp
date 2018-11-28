// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Xml;
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
        Header header;
        DeliveryAnnotations deliveryAnnotations;
        MessageAnnotations messageAnnotations;
        Properties properties;
        ApplicationProperties applicationProperties;
        Footer footer;
        SectionFlag sectionFlags;
        bool disposed;

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
                EnsureInitialized<DeliveryAnnotations>(ref this.deliveryAnnotations, SectionFlag.DeliveryAnnotations);
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
                EnsureInitialized<MessageAnnotations>(ref this.messageAnnotations, SectionFlag.MessageAnnotations);
                return this.messageAnnotations;
            }

            set
            {
                this.messageAnnotations = value;
                this.UpdateSectionFlag(value != null, SectionFlag.MessageAnnotations);
            }
        }

        public Properties Properties
        {
            get
            {
                EnsureInitialized<Properties>(ref this.properties, SectionFlag.Properties);
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
                EnsureInitialized<ApplicationProperties>(ref this.applicationProperties, SectionFlag.ApplicationProperties);
                return this.applicationProperties;
            }

            set
            {
                this.applicationProperties = value;
                this.UpdateSectionFlag(value != null, SectionFlag.ApplicationProperties);
            }
        }

        public virtual IEnumerable<Data> DataBody
        {
            get { throw new InvalidOperationException(); }
            set { throw new InvalidOperationException(); }
        }

        public virtual IEnumerable<AmqpSequence> SequenceBody
        {
            get { throw new InvalidOperationException(); }
            set { throw new InvalidOperationException(); }
        }

        public virtual AmqpValue ValueBody
        {
            get { throw new InvalidOperationException(); }
            set { throw new InvalidOperationException(); }
        }

        public virtual Stream BodyStream
        {
            get { throw new InvalidOperationException(); }
            set { throw new InvalidOperationException(); }
        }
        
        public Footer Footer
        {
            get
            {
                EnsureInitialized<Footer>(ref this.footer, SectionFlag.Footer);
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
                this.Deserialize(SectionFlag.All);
                return this.sectionFlags;
            }
        }

        public SectionFlag BodyType
        {
            get
            {
                this.Deserialize(SectionFlag.All);
                return this.sectionFlags & SectionFlag.Body;
            }
        }

        internal long BodySectionOffset
        {
            get;
            set;
        }

        internal long BodySectionLength
        {
            get;
            set;
        }

        public abstract long SerializedMessageSize
        {
            get;
        }

        public ArraySegment<byte>[] GetPayload()
        {
            bool more;
            return this.GetPayload(int.MaxValue, out more);
        }

        // Public factory methods
        public static AmqpMessage Create()
        {
            return new AmqpEmptyMessage();
        }

        public static AmqpMessage Create(Data data)
        {
            return Create(new Data[] { data });
        }

        public static AmqpMessage Create(IEnumerable<Data> dataList)
        {
            return new AmqpDataMessage(dataList);
        }

        public static AmqpMessage Create(AmqpValue value)
        {
            return new AmqpValueMessage(value);
        }

        public static AmqpMessage Create(IEnumerable<AmqpSequence> amqpSequence)
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
            return new AmqpStreamMessage();
        }

        public static AmqpMessage CreateAmqpStreamMessageBody(Stream bodyStream)
        {
            return new AmqpStreamMessage(BufferListStream.Create(bodyStream, AmqpConstants.SegmentSize));
        }

        public static AmqpMessage CreateAmqpStreamMessageHeader(BufferListStream nonBodyStream)
        {
            return new AmqpStreamMessageHeader(nonBodyStream);
        }

        public static AmqpMessage CreateAmqpStreamMessage(BufferListStream messageStream)
        {
            return new AmqpStreamMessage(messageStream);
        }

        public static AmqpMessage CreateAmqpStreamMessage(BufferListStream messagerStream, bool payloadInitialized)
        {
            return new AmqpStreamMessage(messagerStream, payloadInitialized);
        }

        public static AmqpMessage CreateAmqpStreamMessage(Stream nonBodyStream, Stream bodyStream, bool forceCopyStream)
        {
            return new AmqpStreamMessage(nonBodyStream, bodyStream, forceCopyStream);
        }

        public static AmqpMessage CreateInputMessage(BufferListStream stream)
        {
            return new AmqpInputStreamMessage(stream);
        }

        public static AmqpMessage CreateOutputMessage(BufferListStream stream, bool ownStream)
        {
            return new AmqpOutputStreamMessage(stream, ownStream);
        }

        public AmqpMessage Clone()
        {
            if (this.RawByteBuffers != null)
            {
                ArraySegment<byte>[] segments = new ArraySegment<byte>[this.RawByteBuffers.Count];
                for (int i = 0; i < this.RawByteBuffers.Count; ++i)
                {
                    ByteBuffer clone = this.RawByteBuffers[i].AddReference();
                    segments[i] = new ArraySegment<byte>(clone.Buffer, clone.Offset, clone.Length);
                }

                return new AmqpStreamMessage(new BufferListStream(segments), true);
            }
            else
            {
                if (this.BytesTransfered > 0)
                {
                    throw new InvalidOperationException(AmqpResources.AmqpCannotCloneSentMessage);
                }

                bool more;
                ArraySegment<byte>[] segments = this.GetPayload(int.MaxValue, out more);
                return new AmqpStreamMessage(new BufferListStream(segments), true);
            }
        }

        public void Modify(Modified modified)
        {
            // TODO: handle delivery failed and undeliverable here
            foreach(KeyValuePair<MapKey, object> pair in modified.MessageAnnotations)
            {
                this.MessageAnnotations.Map[pair.Key] = pair.Value;
            }
        }

        public virtual Stream ToStream()
        {
            throw new InvalidOperationException();
        }

        public virtual void Deserialize(SectionFlag desiredSections)
        {
        }

        protected override void Dispose(bool disposing)
        {
            this.disposed = true;
            base.Dispose(disposing);
        }

        public void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException(this.GetType().Name);
            }
        }

        protected virtual void EnsureInitialized<T>(ref T obj, SectionFlag section) where T : class, new()
        {
            if (AmqpMessage.EnsureInitialized(ref obj))
            {
                this.sectionFlags |= section;
            }
        }

        internal virtual long Write(XmlWriter writer)
        {
            throw new InvalidOperationException();
        }

        internal virtual Stream GetBodySectionStream()
        {
            throw new InvalidOperationException();
        }

        internal virtual Stream GetNonBodySectionsStream()
        {
            throw new InvalidOperationException();
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

        int GetSectionSize(IAmqpSerializable section)
        {
            return section == null ? 0 : section.EncodeSize;
        }

        void EncodeSection(ByteBuffer buffer, IAmqpSerializable section)
        {
            if (section != null)
            {
                section.Encode(buffer);
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

        abstract class AmqpBufferedMessage : AmqpMessage
        {
            BufferListStream bufferStream;
            bool initialized;

            public override long SerializedMessageSize
            {
                get
                {
                    if (this.bufferStream != null)
                    {
                        return this.bufferStream.Length;
                    }

                    // Do not cache the result stream
                    // as the message could be updated again
                    return this.Initialize().Length;
                }
            }

            public override ArraySegment<byte>[] GetPayload(int payloadSize, out bool more)
            {
                this.EnsureInitialized();
                return this.bufferStream.ReadBuffers(payloadSize, false, out more);
            }

            protected void EnsureInitialized()
            {
                if (!this.initialized)
                {
                    this.bufferStream = this.Initialize();
                    this.initialized = true;
                }
            }

            protected override void OnCompletePayload(int payloadSize)
            {
                long position = this.bufferStream.Position;
                this.bufferStream.Position = position + payloadSize;
            }

            protected virtual void OnInitialize()
            {
            }

            public override Stream ToStream()
            {
                if (this.bufferStream != null)
                {
                    return (Stream)this.bufferStream.Clone();
                }

                return this.Initialize();
            }

            protected abstract int GetBodySize();

            protected abstract void EncodeBody(ByteBuffer buffer);

            protected virtual void AddCustomSegments(List<ArraySegment<byte>> segmentList)
            {
            }

            BufferListStream Initialize()
            {
                this.OnInitialize();

                int encodeSize = this.GetSectionSize(this.header) +
                    this.GetSectionSize(this.deliveryAnnotations) +
                    this.GetSectionSize(this.messageAnnotations) +
                    this.GetSectionSize(this.properties) +
                    this.GetSectionSize(this.applicationProperties) +
                    this.GetBodySize() +
                    this.GetSectionSize(this.footer);

                List<ArraySegment<byte>> segmentList = new List<ArraySegment<byte>>(4);
                if (encodeSize == 0)
                {
                    this.AddCustomSegments(segmentList);
                }
                else
                {
                    ByteBuffer buffer = new ByteBuffer(new byte[encodeSize]);
                    int segmentOffset = 0;

                    this.EncodeSection(buffer, this.header);
                    this.EncodeSection(buffer, this.deliveryAnnotations);
                    this.EncodeSection(buffer, this.messageAnnotations);
                    this.EncodeSection(buffer, this.properties);
                    this.EncodeSection(buffer, this.applicationProperties);
                    if (buffer.Length > 0)
                    {
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, buffer.Length));
                    }

                    segmentOffset = buffer.Length;
                    this.EncodeBody(buffer);
                    int count = buffer.Length - segmentOffset;
                    if (count > 0)
                    {
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, count));
                    }

                    this.AddCustomSegments(segmentList);

                    if (this.footer != null)
                    {
                        segmentOffset = buffer.Length;
                        this.EncodeSection(buffer, this.footer);
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, buffer.Length - segmentOffset));
                    }
                }

                return new BufferListStream(segmentList.ToArray());
            }
        }

        sealed class AmqpEmptyMessage : AmqpBufferedMessage
        {
            protected override int GetBodySize()
            {
                return 0;
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
            }
        }

        sealed class AmqpValueMessage : AmqpBufferedMessage
        {
            readonly AmqpValue value;

            public AmqpValueMessage(AmqpValue value)
            {
                this.value = value;
                this.sectionFlags |= SectionFlag.AmqpValue;
            }

            public override AmqpValue ValueBody
            {
                get { return this.value; }
            }

            protected override int GetBodySize()
            {
                return this.GetSectionSize(this.value);
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
                this.EncodeSection(buffer, this.value);
            }
        }

        sealed class AmqpDataMessage : AmqpBufferedMessage
        {
            readonly IEnumerable<Data> dataList;

            public AmqpDataMessage(IEnumerable<Data> dataList)
            {
                this.dataList = dataList;
                this.sectionFlags |= SectionFlag.Data;
            }

            public override IEnumerable<Data> DataBody
            {
                get { return this.dataList; }
            }

            public override Stream BodyStream
            {
                get { return new BufferListStream(this.dataList.Select(d => (ArraySegment<byte>) d.Value).ToArray()); }
            }

            protected override int GetBodySize()
            {
                return 0;
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
            }

            protected override void AddCustomSegments(List<ArraySegment<byte>> segmentList)
            {
                foreach (Data data in this.dataList)
                {
                    ArraySegment<byte> value = (ArraySegment<byte>)data.Value;
                    segmentList.Add(Data.GetEncodedPrefix(value.Count));
                    segmentList.Add(value);
                }
            }
        }

        sealed class AmqpSequenceMessage : AmqpBufferedMessage
        {
            readonly IEnumerable<AmqpSequence> sequence;

            public AmqpSequenceMessage(IEnumerable<AmqpSequence> sequence)
            {
                this.sequence = sequence;
                this.sectionFlags |= SectionFlag.AmqpSequence;
            }

            public override IEnumerable<AmqpSequence> SequenceBody
            {
                get { return this.sequence; }
            }

            protected override int GetBodySize()
            {
                int bodySize = 0;
                foreach (AmqpSequence seq in this.sequence)
                {
                    bodySize += this.GetSectionSize(seq);
                }

                return bodySize;
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
                foreach (AmqpSequence seq in this.sequence)
                {
                    this.EncodeSection(buffer, seq);
                }
            }
        }

        /// <summary>
        /// Wraps a stream in the message body. The data is sent in one or more Data sections.
        /// </summary>
        sealed class AmqpBodyStreamMessage : AmqpBufferedMessage
        {
            readonly Stream bodyStream;
            readonly bool ownStream;
            ArraySegment<byte>[] bodyData;
            int bodyLength;

            public AmqpBodyStreamMessage(Stream bodyStream, bool ownStream)
            {
                Fx.Assert(bodyStream != null, "The bodyStream argument should not be null.");
                this.sectionFlags |= SectionFlag.Data;
                this.bodyStream = bodyStream;
                this.ownStream = ownStream;
            }

            public override Stream BodyStream
            {
                get
                {
                    this.EnsureInitialized();
                    return new BufferListStream(this.bodyData);
                }

                set
                {
                    base.BodyStream = value;
                }
            }

            protected override void OnInitialize()
            {
                if (this.bodyData == null)
                {
                    this.bodyData = BufferListStream.ReadStream(this.bodyStream, 1024, out this.bodyLength);
                    if (this.ownStream)
                    {
                        this.bodyStream.Dispose();
                    }
                }
            }

            protected override int GetBodySize()
            {
                return 0;
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
            }

            protected override void AddCustomSegments(List<ArraySegment<byte>> segmentList)
            {
                // if body length is 0, send an empty Data section
                segmentList.Add(Data.GetEncodedPrefix(this.bodyLength));
                if (this.bodyLength > 0)
                {
                    segmentList.AddRange(this.bodyData);
                }
            }
        }

        /// <summary>
        /// The stream contains an entire AMQP message. When the stream is sent out,
        /// the mutable sections are updated.
        /// </summary>
        sealed class AmqpOutputStreamMessage : AmqpBufferedMessage
        {
            readonly BufferListStream messageStream;
            readonly bool ownStream;
            ArraySegment<byte>[] buffers;

            public AmqpOutputStreamMessage(BufferListStream messageStream, bool ownStream)
            {
                this.messageStream = messageStream;
                this.ownStream = ownStream;
            }

            protected override void OnInitialize()
            {
                BufferListStream stream = this.messageStream;
                if (!this.ownStream)
                {
                    stream = (BufferListStream)stream.Clone();
                }

                try
                {
                    // backup and cleanup
                    Header savedHeader = this.header;
                    DeliveryAnnotations savedDeliveryAnnotation = this.deliveryAnnotations;
                    MessageAnnotations savedMessageAnnotation = this.messageAnnotations;
                    this.header = null;
                    this.deliveryAnnotations = null;
                    this.messageAnnotations = null;

                    AmqpMessageReader reader = new AmqpMessageReader(stream);
                    reader.ReadMessage(this, SectionFlag.Header | SectionFlag.DeliveryAnnotations | SectionFlag.MessageAnnotations);

                    // merge
                    this.UpdateHeader(savedHeader);
                    this.UpdateDeliveryAnnotations(savedDeliveryAnnotation);
                    this.UpdateMessageAnnotations(savedMessageAnnotation);

                    // mask off immutable sections except footer
                    this.properties = null;
                    this.applicationProperties = null;
                    this.footer = null;

                    // read out the remaining buffers
                    bool unused = false;
                    this.buffers = stream.ReadBuffers(int.MaxValue, true, out unused);
                }
                finally
                {
                    if (!this.ownStream)
                    {
                        stream.Dispose();
                    }
                }
            }

            protected override int GetBodySize()
            {
                return 0;
            }

            protected override void EncodeBody(ByteBuffer buffer)
            {
            }

            protected override void AddCustomSegments(List<ArraySegment<byte>> segmentList)
            {
                if (this.buffers != null && this.buffers.Length > 0)
                {
                    segmentList.AddRange(this.buffers);
                }
            }

            void UpdateHeader(Header modified)
            {
                if (modified != null)
                {
                    if (this.header == null)
                    {
                        this.Header = modified;
                    }
                    else
                    {
                        // update this header only if it is null
                        if (modified.Durable != null) this.header.Durable = modified.Durable;
                        if (modified.Priority != null) this.header.Priority = this.header.Priority;
                        if (modified.Ttl != null) this.header.Ttl = modified.Ttl;
                        if (modified.FirstAcquirer != null) this.header.FirstAcquirer = modified.FirstAcquirer;
                        if (modified.DeliveryCount != null) this.header.DeliveryCount = modified.DeliveryCount;
                    }
                }
            }

            void UpdateDeliveryAnnotations(DeliveryAnnotations modified)
            {
                if (modified != null)
                {
                    if (this.deliveryAnnotations == null)
                    {
                        this.DeliveryAnnotations = modified;
                    }
                    else
                    {
                        foreach (KeyValuePair<MapKey, object> pair in modified.Map)
                        {
                            this.deliveryAnnotations.Map[pair.Key] = pair.Value;
                        }
                    }
                }
            }

            void UpdateMessageAnnotations(MessageAnnotations modified)
            {
                if (modified != null)
                {
                    if (this.messageAnnotations == null)
                    {
                        this.MessageAnnotations = modified;
                    }
                    else
                    {
                        foreach (KeyValuePair<MapKey, object> pair in modified.Map)
                        {
                            this.messageAnnotations.Map[pair.Key] = pair.Value;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Used on the receiver side. The entire message is immutable (changes are discarded).
        /// </summary>
        sealed class AmqpInputStreamMessage : AmqpMessage
        {
            readonly BufferListStream bufferStream;
            bool deserialized;
            IEnumerable<Data> dataList;
            IEnumerable<AmqpSequence> sequenceList;
            AmqpValue amqpValue;
            Stream bodyStream;

            public AmqpInputStreamMessage(BufferListStream bufferStream)
            {
                this.bufferStream = bufferStream;
            }

            public override IEnumerable<Data> DataBody
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.dataList;
                }

                set
                {
                    this.dataList = value;
                    this.UpdateSectionFlag(value != null, SectionFlag.Data);
                }
            }

            public override IEnumerable<AmqpSequence> SequenceBody
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.sequenceList;
                }

                set
                {
                    this.sequenceList = value;
                    this.UpdateSectionFlag(value != null, SectionFlag.AmqpSequence);
                }
            }

            public override AmqpValue ValueBody
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.amqpValue;
                }

                set
                {
                    this.amqpValue = value;
                    this.UpdateSectionFlag(value != null, SectionFlag.AmqpValue);
                }
            }

            public override Stream BodyStream
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.bodyStream;
                }

                set
                {
                    this.bodyStream = value;
                }
            }

            public override long SerializedMessageSize
            {
                get
                {
                    return this.bufferStream.Length;
                }
            }

            public override ArraySegment<byte>[] GetPayload(int payloadSize, out bool more)
            {
                throw new InvalidOperationException();
            }

            protected override void OnCompletePayload(int payloadSize)
            {
                throw new InvalidOperationException();
            }

            protected override void EnsureInitialized<T>(ref T obj, SectionFlag section)
            {
                this.Deserialize(SectionFlag.All);
            }

            public override Stream ToStream()
            {
                return this.bufferStream;
            }

            public override void Deserialize(SectionFlag desiredSections)
            {
                if (!this.deserialized)
                {
                    BufferListStream stream = (BufferListStream)this.bufferStream.Clone();
                    AmqpMessageReader reader = new AmqpMessageReader(stream);
                    reader.ReadMessage(this, desiredSections);
                    stream.Dispose();
                    this.deserialized = true;
                }
            }
        }

        sealed class AmqpMessageReader
        {
            static Dictionary<string, ulong> sectionCodeByName = new Dictionary<string, ulong>()
            {
                { Header.Name, Header.Code },
                { DeliveryAnnotations.Name, DeliveryAnnotations.Code },
                { MessageAnnotations.Name, MessageAnnotations.Code },
                { Properties.Name, Properties.Code },
                { ApplicationProperties.Name, ApplicationProperties.Code },
                { Data.Name, Data.Code },
                { AmqpSequence.Name, AmqpSequence.Code },
                { AmqpValue.Name, AmqpValue.Code },
                { Footer.Name, Footer.Code },
            };

            static Action<AmqpMessageReader, AmqpMessage, long>[] sectionReaders = new Action<AmqpMessageReader, AmqpMessage, long>[]
            {
                ReadHeaderSection,
                ReadDeliveryAnnotationsSection,
                ReadMessageAnnotationsSection,
                ReadPropertiesSection,
                ReadApplicationPropertiesSection,
                ReadDataSection,
                ReadAmqpSequenceSection,
                ReadAmqpValueSection,
                ReadFooterSection,
            };

            readonly BufferListStream stream;
            List<Data> dataList;
            List<AmqpSequence> sequenceList;
            AmqpValue amqpValue;
            List<ArraySegment<byte>> bodyBuffers;

            public AmqpMessageReader(BufferListStream stream)
            {
                this.stream = stream;
            }

            public void ReadMessage(AmqpMessage message, SectionFlag sections)
            {
                while (this.ReadSection(message, sections));

                if ((sections & SectionFlag.Body) != 0)
                {
                    if (this.dataList != null)
                    {
                        message.DataBody = this.dataList;
                    }

                    if (this.sequenceList != null)
                    {
                        message.SequenceBody = this.sequenceList;
                    }

                    if (this.amqpValue != null)
                    {
                        message.ValueBody = this.amqpValue;
                    }

                    if (this.bodyBuffers != null)
                    {
                        message.BodyStream = new BufferListStream(this.bodyBuffers.ToArray());
                    }
                }
            }

            static void ReadHeaderSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                message.Header = ReadListSection<Header>(reader);
                message.header.Offset = startPosition;
                message.header.Length = reader.stream.Position - startPosition;
            }

            static void ReadDeliveryAnnotationsSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                message.DeliveryAnnotations = ReadMapSection<DeliveryAnnotations>(reader);
                message.deliveryAnnotations.Offset = startPosition;
                message.deliveryAnnotations.Length = reader.stream.Position - startPosition;
            }

            static void ReadMessageAnnotationsSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                message.MessageAnnotations = ReadMapSection<MessageAnnotations>(reader);
                message.messageAnnotations.Offset = startPosition;
                message.messageAnnotations.Length = reader.stream.Position - startPosition;
            }

            static void ReadPropertiesSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                message.Properties = ReadListSection<Properties>(reader);
                message.properties.Offset = startPosition;
                message.properties.Length = reader.stream.Position - startPosition;
            }

            static void ReadApplicationPropertiesSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                message.ApplicationProperties = ReadMapSection<ApplicationProperties>(reader);
                message.applicationProperties.Offset = startPosition;
                message.applicationProperties.Length = reader.stream.Position - startPosition;
            }

            static void ReadDataSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                FormatCode formatCode = reader.ReadFormatCode();
                Fx.Assert(formatCode == FormatCode.Binary8 || formatCode == FormatCode.Binary32, "Invalid binary format code");
                bool smallEncoding = formatCode == FormatCode.Binary8;
                int count = reader.ReadInt(smallEncoding);
                ArraySegment<byte> buffer;
                if (count > 0)
                {
                    buffer = reader.ReadBytes(count);
                }
                else
                {
                    buffer = AmqpConstants.EmptyBinary;
                }

                AmqpMessage.EnsureInitialized<List<Data>>(ref reader.dataList);
                reader.dataList.Add(new Data() { Value = buffer });

                reader.AddBodyBuffer(buffer);
            }

            static void ReadAmqpSequenceSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                AmqpMessage.EnsureInitialized<List<AmqpSequence>>(ref reader.sequenceList);
                reader.sequenceList.Add(ReadListSection<AmqpSequence>(reader, true));
            }

            static void ReadAmqpValueSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                ArraySegment<byte> buffer = reader.ReadBytes(int.MaxValue);
                ByteBuffer byteBuffer = new ByteBuffer(buffer);
                object value = AmqpCodec.DecodeObject(byteBuffer);
                reader.amqpValue = new AmqpValue() { Value = value };

                reader.AddBodyBuffer(buffer);

                // we didn't know the size and the buffer may include the footer
                if (byteBuffer.Length > 0)
                {
                    int footerLength = byteBuffer.Length;
                    Footer footer = new Footer();
                    footer.Decode(byteBuffer);
                    message.Footer = footer;
                    message.footer.Offset = reader.stream.Position - footerLength;
                    message.footer.Length = footerLength;
                }
            }

            static void ReadFooterSection(AmqpMessageReader reader, AmqpMessage message, long startPosition)
            {
                message.Footer = ReadMapSection<Footer>(reader);
                message.footer.Offset = startPosition;
                message.footer.Length = reader.stream.Position - startPosition;
            }

            static T ReadListSection<T>(AmqpMessageReader reader, bool isBodySection = false) where T : DescribedList, new()
            {
                T section = new T();
                long position = reader.stream.Position;
                FormatCode formatCode = reader.ReadFormatCode();
                Fx.Assert(formatCode == FormatCode.List8 || formatCode == FormatCode.List0 || formatCode == FormatCode.List32, "Invalid list format code");
                if (formatCode == FormatCode.List0)
                {
                    return section;
                }

                bool smallEncoding = formatCode == FormatCode.List8;
                int size = reader.ReadInt(smallEncoding);
                int count = reader.ReadInt(smallEncoding);
                if (count == 0)
                {
                    return section;
                }

                long position2 = reader.stream.Position;

                ArraySegment<byte> bytes = reader.ReadBytes(size - (smallEncoding ? FixedWidth.UByte : FixedWidth.UInt));
                long position3 = reader.stream.Position;

                section.DecodeValue(new ByteBuffer(bytes), size, count);

                // Check if we are decoding the AMQP value body
                if (isBodySection)
                {
                    reader.stream.Position = position;
                    ArraySegment<byte> segment = reader.stream.ReadBytes((int)(position2 - position));
                    reader.stream.Position = position3;

                    reader.AddBodyBuffer(segment);
                    reader.AddBodyBuffer(bytes);
                }

                return section;
            }

            static T ReadMapSection<T>(AmqpMessageReader reader) where T : DescribedMap, new()
            {
                T section = new T();
                FormatCode formatCode = reader.ReadFormatCode();
                Fx.Assert(formatCode == FormatCode.Map8 || formatCode == FormatCode.Map32, "Invalid map format code");
                bool smallEncoding = formatCode == FormatCode.Map8;
                int size = reader.ReadInt(smallEncoding);
                int count = reader.ReadInt(smallEncoding);
                if (count > 0)
                {
                    ArraySegment<byte> bytes = reader.ReadBytes(size - (smallEncoding ? FixedWidth.UByte : FixedWidth.UInt));
                    section.DecodeValue(new ByteBuffer(bytes), size, count);
                }

                return section;
            }

            bool ReadSection(AmqpMessage message, SectionFlag sections)
            {
                long position = this.stream.Position;
                if (position == this.stream.Length)
                {
                    return false;
                }

                FormatCode formatCode = this.ReadFormatCode();
                if (formatCode != FormatCode.Described)
                {
                    throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, this.stream.Position));
                }

                ulong descriptorCode = this.ReadDescriptorCode();
                if (descriptorCode < Header.Code || descriptorCode > Footer.Code)
                {
                    throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidMessageSectionCode, descriptorCode));
                }

                int sectionIndex = (int)(descriptorCode - Header.Code);
                SectionFlag sectionFlag = (SectionFlag)(1 << sectionIndex);
                if ((sectionFlag & sections) == 0)
                {
                    // The section we want to decode does not exist, so rollback to
                    // where we were.
                    this.stream.Position = position;
                    return false;
                }

                sectionReaders[sectionIndex](this, message, position);

                if ((sectionFlag & SectionFlag.Body) != 0)
                {
                    message.BodySectionOffset = position;
                    message.BodySectionLength = this.stream.Position - position;
                }

                return true;
            }

            FormatCode ReadFormatCode()
            {
                byte type = (byte)this.stream.ReadByte();
                byte extType = 0;
                if (FormatCode.HasExtType(type))
                {
                    extType = (byte)this.stream.ReadByte();
                }

                return new FormatCode(type, extType);
            }

            ulong ReadDescriptorCode()
            {
                FormatCode formatCode = this.ReadFormatCode();
                ulong descriptorCode = 0;
                if (formatCode == FormatCode.SmallULong)
                {
                    descriptorCode = (ulong)this.stream.ReadByte();
                }
                else if (formatCode == FormatCode.ULong)
                {
                    ArraySegment<byte> buffer = this.ReadBytes(FixedWidth.ULong);
                    descriptorCode = AmqpBitConverter.ReadULong(buffer.Array, buffer.Offset, FixedWidth.ULong);
                }
                else if (formatCode == FormatCode.Symbol8 || formatCode == FormatCode.Symbol32)
                {
                    int count = this.ReadInt(formatCode == FormatCode.Symbol8);
                    ArraySegment<byte> nameBuffer = this.ReadBytes(count);
                    string descriptorName = Platform.System.Text.Encoding.ASCII.GetString(nameBuffer.Array, nameBuffer.Offset, count);
                    sectionCodeByName.TryGetValue(descriptorName, out descriptorCode);
                }

                return descriptorCode;
            }

            int ReadInt(bool smallEncoding)
            {
                if (smallEncoding)
                {
                    return this.stream.ReadByte();
                }
                else
                {
                    ArraySegment<byte> buffer = this.ReadBytes(FixedWidth.UInt);
                    return(int)AmqpBitConverter.ReadUInt(buffer.Array, buffer.Offset, FixedWidth.UInt);
                }
            }

            ArraySegment<byte> ReadBytes(int count)
            {
                ArraySegment<byte> bytes = this.stream.ReadBytes(count);
                if (count != int.MaxValue && bytes.Count < count)
                {
                    throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInsufficientBufferSize, count, bytes.Count));
                }

                return bytes;
            }

            void AddBodyBuffer(ArraySegment<byte> buffer)
            {
                AmqpMessage.EnsureInitialized<List<ArraySegment<byte>>>(ref this.bodyBuffers);
                if (buffer.Count > 0)
                {
                    this.bodyBuffers.Add(buffer);
                }
            }
        }

        sealed class AmqpStreamMessage : AmqpMessage
        {
            readonly BufferListStream bodySection;
            BufferListStream messageStream;
            BufferListStream payloadStream;
            IEnumerable<Data> dataList;
            IEnumerable<AmqpSequence> sequenceList;
            AmqpValue amqpValue;
            Stream bodyStream;
            bool payloadInitialized;
            bool deserialized;

            public AmqpStreamMessage()
            {
                this.RawByteBuffers = new List<ByteBuffer>();
            }

            public AmqpStreamMessage(BufferListStream messageStream)
                : this(messageStream, false)
            {
            }

            public AmqpStreamMessage(BufferListStream messageStream, bool payloadInitialized)
            {
                if (messageStream == null)
                {
                    throw new ArgumentNullException(nameof(messageStream));
                }

                this.messageStream = messageStream;
                this.payloadInitialized = payloadInitialized;
                if (payloadInitialized)
                {
                    this.payloadStream = this.messageStream;
                }
            }

            public AmqpStreamMessage(Stream nonBodySections, Stream bodySection, bool forceCopyStream)
            {
                // Currently message always has header stream, may change in the future
                if (nonBodySections == null)
                {
                    throw new ArgumentNullException(nameof(nonBodySections));
                }

                this.messageStream = BufferListStream.Create(nonBodySections, AmqpConstants.SegmentSize, forceCopyStream);
                if (bodySection != null)
                {
                    this.bodySection = BufferListStream.Create(bodySection, AmqpConstants.SegmentSize, forceCopyStream);
                }
            }

            public override long SerializedMessageSize
            {
                get
                {
                    return this.payloadInitialized ?
                        this.payloadStream.Length :
                        this.messageStream.Length + (this.bodySection == null ? 0 : this.bodySection.Length);
                }
            }

            public override ArraySegment<byte>[] GetPayload(int payloadSize, out bool more)
            {
                if (!this.payloadInitialized)
                {
                    this.payloadStream = AmqpStreamMessage.Encode(this);
                    this.payloadInitialized = true;
                }

                return this.payloadStream.ReadBuffers(payloadSize, false, out more);
            }

            public override void AddPayload(ByteBuffer payload, bool isLast)
            {
                Fx.Assert(this.RawByteBuffers != null, "Raw buffer list must have been initialized!");
                this.RawByteBuffers.Add(payload);
                this.BytesTransfered += payload.Length;
                if (isLast)
                {
                    ArraySegment<byte>[] segments = new ArraySegment<byte>[this.RawByteBuffers.Count];
                    for (int i = 0; i < this.RawByteBuffers.Count; ++i)
                    {
                        ByteBuffer byteBuffer = this.RawByteBuffers[i];
                        segments[i] = new ArraySegment<byte>(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
                    }

                    this.messageStream = new BufferListStream(segments);
                }
            }

            protected override void OnCompletePayload(int payloadSize)
            {
                long position = this.payloadStream.Position;
                this.payloadStream.Position = position + payloadSize;
            }

            public override IEnumerable<Data> DataBody
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.dataList;
                }

                set
                {
                    this.dataList = value;
                    this.UpdateSectionFlag(value != null, SectionFlag.Data);
                }
            }

            public override IEnumerable<AmqpSequence> SequenceBody
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.sequenceList;
                }

                set
                {
                    this.sequenceList = value;
                    this.UpdateSectionFlag(value != null, SectionFlag.AmqpSequence);
                }
            }

            public override AmqpValue ValueBody
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.amqpValue;
                }

                set
                {
                    this.amqpValue = value;
                    this.UpdateSectionFlag(value != null, SectionFlag.AmqpValue);
                }
            }

            public override Stream BodyStream
            {
                get
                {
                    this.Deserialize(SectionFlag.All);
                    return this.bodyStream;
                }

                set
                {
                    this.bodyStream = value;
                }
            }

            protected override void EnsureInitialized<T>(ref T obj, SectionFlag section)
            {
                this.Deserialize(SectionFlag.All);
            }

            public override Stream ToStream()
            {
                return this.messageStream;
            }

            public override void Deserialize(SectionFlag desiredSections)
            {
                if (!this.deserialized)
                {
                    BufferListStream stream = (BufferListStream)this.messageStream.Clone();
                    AmqpMessageReader reader = new AmqpMessageReader(stream);
                    reader.ReadMessage(this, desiredSections);
                    stream.Dispose();

                    this.header = this.header ?? new Header();
                    this.messageAnnotations = this.messageAnnotations ?? new MessageAnnotations();

                    this.deserialized = true;
                }
            }

            internal override Stream GetNonBodySectionsStream()
            {
                this.Deserialize(SectionFlag.All);
                using (BufferListStream stream = (BufferListStream)this.messageStream.Clone())
                {
                    List<ArraySegment<byte>> segments = new List<ArraySegment<byte>>();
                    ReadSection(stream, segments, this.Header);
                    ReadSection(stream, segments, this.DeliveryAnnotations);
                    ReadSection(stream, segments, this.MessageAnnotations);
                    ReadSection(stream, segments, this.Properties);
                    ReadSection(stream, segments, this.ApplicationProperties);
                    ReadSection(stream, segments, this.Footer);

                    return new BufferListStream(segments.ToArray());
                }
            }

            internal override Stream GetBodySectionStream()
            {
                if (this.bodySection != null)
                {
                    return (BufferListStream)this.bodySection.Clone();
                }

                this.Deserialize(SectionFlag.All);
                if ((this.sectionFlags & SectionFlag.Body) == 0)
                {
                    return null;
                }

                using (BufferListStream stream = (BufferListStream)this.messageStream.Clone())
                {
                    bool more;
                    stream.Position = this.BodySectionOffset;
                    ArraySegment<byte>[] segments = stream.ReadBuffers((int)this.BodySectionLength, true, out more);
                    return new BufferListStream(segments);
                }
            }

            ArraySegment<byte>[] GetBodySectionSegments()
            {
                if (this.bodySection != null)
                {
                    using (BufferListStream stream = (BufferListStream)this.bodySection.Clone())
                    {
                        bool more;
                        return stream.ReadBuffers(int.MaxValue, false, out more);
                    }
                }

                this.Deserialize(SectionFlag.All);
                if ((this.sectionFlags & SectionFlag.Body) == 0)
                {
                    return null;
                }

                using (BufferListStream stream = (BufferListStream)this.messageStream.Clone())
                {
                    bool more;
                    stream.Position = this.BodySectionOffset;
                    return stream.ReadBuffers((int)this.BodySectionLength, true, out more);
                }
            }

            void ReadSection(BufferListStream source, List<ArraySegment<byte>> target, AmqpDescribed section)
            {
                if (section != null && section.Length > 0)
                {
                    bool more;
                    source.Position = section.Offset;
                    ArraySegment<byte>[] segments = source.ReadBuffers((int)section.Length, false, out more);
                    if (segments != null && segments.Length > 0)
                    {
                        target.AddRange(segments);
                    }
                }
            }

            void EncodeSection(ByteBuffer buffer, AmqpDescribed section)
            {
                if (section != null)
                {
                    section.Offset = buffer.Length;
                    base.EncodeSection(buffer, section);
                    section.Length = buffer.Length - section.Offset;
                }
            }

            static BufferListStream Encode(AmqpStreamMessage message)
            {
                int encodeSize = message.GetSectionSize(message.header) +
                    message.GetSectionSize(message.deliveryAnnotations) +
                    message.GetSectionSize(message.messageAnnotations) +
                    message.GetSectionSize(message.properties) +
                    message.GetSectionSize(message.applicationProperties) +
                    message.GetSectionSize(message.footer);

                List<ArraySegment<byte>> segmentList = new List<ArraySegment<byte>>(4);
                {
                    ByteBuffer buffer = new ByteBuffer(new byte[encodeSize]);
                    int segmentOffset = 0;

                    message.EncodeSection(buffer, message.header);
                    message.EncodeSection(buffer, message.deliveryAnnotations);
                    message.EncodeSection(buffer, message.messageAnnotations);
                    message.EncodeSection(buffer, message.properties);
                    message.EncodeSection(buffer, message.applicationProperties);
                    if (buffer.Length > 0)
                    {
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, buffer.Length));
                    }

                    var bodySegments = message.GetBodySectionSegments();
                    if (bodySegments != null)
                    {
                        segmentList.AddRange(bodySegments);
                    }

                    if (message.footer != null)
                    {
                        segmentOffset = buffer.Length;
                        message.EncodeSection(buffer, message.footer);
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, buffer.Length - segmentOffset));
                    }
                }

                return new BufferListStream(segmentList.ToArray());
            }
        }

        sealed class AmqpStreamMessageHeader : AmqpMessage
        {
            readonly BufferListStream bufferStream;
            bool deserialized;

            public AmqpStreamMessageHeader(BufferListStream headerStream)
            {
                if (headerStream == null)
                {
                    throw new ArgumentNullException(nameof(headerStream));
                }

                this.bufferStream = headerStream;
            }

            public override long SerializedMessageSize
            {
                get
                {
                    return this.bufferStream.Length;
                }
            }

            public override ArraySegment<byte>[] GetPayload(int payloadSize, out bool more)
            {
                throw new InvalidOperationException();
            }

            protected override void OnCompletePayload(int payloadSize)
            {
                throw new InvalidOperationException();
            }

            protected override void EnsureInitialized<T>(ref T obj, SectionFlag section)
            {
                this.Deserialize(SectionFlag.All);
            }

            public override Stream ToStream()
            {
                return this.bufferStream;
            }

            internal override Stream GetNonBodySectionsStream()
            {
                return (BufferListStream)this.bufferStream.Clone();
            }

            public override void Deserialize(SectionFlag desiredSections)
            {
                if (!this.deserialized)
                {
                    BufferListStream stream = (BufferListStream)this.bufferStream.Clone();
                    AmqpMessageReader reader = new AmqpMessageReader(stream);
                    reader.ReadMessage(this, desiredSections);
                    stream.Dispose();

                    this.header = this.header ?? new Header();
                    this.deliveryAnnotations = this.deliveryAnnotations ?? new DeliveryAnnotations();
                    this.messageAnnotations = this.messageAnnotations ?? new MessageAnnotations();

                    this.deserialized = true;
                }
            }

            internal override long Write(XmlWriter writer)
            {
                this.Deserialize(SectionFlag.All);
                List<ArraySegment<byte>> updatedSections = AmqpStreamMessageHeader.Encode(this);
                using (BufferListStream stream = new BufferListStream(updatedSections.ToArray()))
                {
                    long size = 0;
                    size += Write(stream, writer, this.Header);
                    size += Write(stream, writer, this.DeliveryAnnotations);
                    size += Write(stream, writer, this.MessageAnnotations);
                    size += Write(stream, writer, this.Properties);
                    size += Write(stream, writer, this.ApplicationProperties);
                    size += Write(stream, writer, this.Footer);

                    return size;
                }
            }

            void EncodeSection(ByteBuffer buffer, AmqpDescribed section)
            {
                if (section != null)
                {
                    section.Offset = buffer.Length;
                    base.EncodeSection(buffer, section);
                    section.Length = buffer.Length - section.Offset;
                }
            }

            static long Write(BufferListStream source, XmlWriter target, AmqpDescribed section)
            {
                long size = 0;

                if (section != null && section.Length > 0)
                {
                    source.Position = section.Offset;
                    byte[] buffer = new byte[section.Length];
                    int readBytes = source.Read(buffer, 0, (int)buffer.Length);
                    target.WriteBase64(buffer, 0, readBytes);
                    size = buffer.Length;
                }

                return size;
            }

            static List<ArraySegment<byte>> Encode(AmqpStreamMessageHeader message)
            {
                int encodeSize =
                    message.GetSectionSize(message.header) +
                    message.GetSectionSize(message.deliveryAnnotations) +
                    message.GetSectionSize(message.messageAnnotations) +
                    message.GetSectionSize(message.properties) +
                    message.GetSectionSize(message.applicationProperties) +
                    message.GetSectionSize(message.footer);

                List<ArraySegment<byte>> segmentList = new List<ArraySegment<byte>>(4);
                {
                    ByteBuffer buffer = new ByteBuffer(new byte[encodeSize]);
                    int segmentOffset = 0;

                    message.EncodeSection(buffer, message.header);
                    message.EncodeSection(buffer, message.deliveryAnnotations);
                    message.EncodeSection(buffer, message.messageAnnotations);
                    message.EncodeSection(buffer, message.properties);
                    message.EncodeSection(buffer, message.applicationProperties);
                    if (buffer.Length > 0)
                    {
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, buffer.Length));
                    }

                    segmentOffset = buffer.Length;
                    int count = buffer.Length - segmentOffset;
                    if (count > 0)
                    {
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, count));
                    }

                    if (message.footer != null)
                    {
                        segmentOffset = buffer.Length;
                        message.EncodeSection(buffer, message.footer);
                        segmentList.Add(new ArraySegment<byte>(buffer.Buffer, segmentOffset, buffer.Length - segmentOffset));
                    }
                }

                return segmentList;
            }
        }
    }
}
