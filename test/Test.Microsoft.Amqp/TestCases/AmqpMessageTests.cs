namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AmqpMessageTests
    {
        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
        }

        [TestMethod()]
        public void AmqpMessageSerializationTest()
        {
            // empty message
            AmqpMessage message = AmqpMessage.Create();
            AddSection(message, SectionFlag.Properties);
            RunSerializationTest(message);

            // data message
            message = AmqpMessage.Create(new Data() { Value = new ArraySegment<byte>(new byte[60]) });
            RunSerializationTest(message);

            message = AmqpMessage.Create(new Data[] { new Data() { Value = new ArraySegment<byte>(new byte[60]) }, new Data() { Value = new ArraySegment<byte>(new byte[44]) } });
            AddSection(message, SectionFlag.Header | SectionFlag.ApplicationProperties);
            RunSerializationTest(message);

            // value message
            message = AmqpMessage.Create(new AmqpValue() { Value = new AmqpSymbol("symbol value") });
            AddSection(message, SectionFlag.Header | SectionFlag.DeliveryAnnotations | SectionFlag.ApplicationProperties);
            RunSerializationTest(message);

            // sequence message
            message = AmqpMessage.Create(new AmqpSequence[] { new AmqpSequence(new List<object>() { "string1", 1234 }), new AmqpSequence(new List<object>() { DateTime.Parse("2012-01-01 12:00:00").ToUniversalTime() }) });
            AddSection(message, SectionFlag.MessageAnnotations | SectionFlag.Properties | SectionFlag.ApplicationProperties | SectionFlag.Footer);
            RunSerializationTest(message);

            // data message - binary
            message = AmqpUtils.CreateMessage(new byte[888]);
            AddSection(message, SectionFlag.DeliveryAnnotations | SectionFlag.ApplicationProperties | SectionFlag.Footer);
            RunSerializationTest(message);

            // body stream message
            message = AmqpMessage.Create(new MemoryStream(new byte[679]), true);
            AddSection(message, SectionFlag.Header | SectionFlag.MessageAnnotations | SectionFlag.Footer);
            RunSerializationTest(message);

            // the following simulates a message's round trip from client to broker to client
            // message -serialize-> buffers -> input stream message -> output stream message -deserialize> message
            message = AmqpMessage.Create(new AmqpValue() { Value = new AmqpSymbol("symbol value") });
            AddSection(message, SectionFlag.MessageAnnotations | SectionFlag.Properties | SectionFlag.ApplicationProperties);
            // serialize - send the message on client side
            ArraySegment<byte>[] buffers = ReadMessagePayLoad(message, 71);
            // input stream message - received on broker side
            AmqpMessage message2 = AmqpMessage.CreateAmqpStreamMessage(new BufferListStream(buffers));
            // output stream message - send out on broker side
            message2 = AmqpMessage.CreateOutputMessage((BufferListStream)message2.ToStream(), true);
            AddSection(message2, SectionFlag.Header | SectionFlag.DeliveryAnnotations);
            message2.MessageAnnotations.Map["delivery-count"] = 5;
            // input stream message - received on client side
            buffers = ReadMessagePayLoad(message2, 71);
            message2 = AmqpMessage.CreateAmqpStreamMessage(new BufferListStream(buffers));
            // update the original message to match the updated message
            AddSection(message, SectionFlag.Header | SectionFlag.DeliveryAnnotations);
            message.MessageAnnotations.Map["delivery-count"] = 5;
            ValidateMessage(message, message2);
        }

        static ArraySegment<byte>[] ReadMessagePayLoad(AmqpMessage message, int payloadSize)
        {
            List<ArraySegment<byte>> buffers = new List<ArraySegment<byte>>();
            bool more = true;
            while (more)
            {
                ArraySegment<byte>[] messageBuffers = message.GetPayload(payloadSize, out more);
                if (messageBuffers != null)
                {
                    foreach (var segment in messageBuffers) { message.CompletePayload(segment.Count); }
                    buffers.AddRange(messageBuffers);
                }
            }

            return buffers.ToArray();
        }

        static void AddSection(AmqpMessage message, SectionFlag sections)
        {
            if ((sections & SectionFlag.Header) != 0)
            {
                message.Header.Priority = 10;
                message.Header.Ttl = (uint)(DateTime.Parse("2011-02-01 12:00:00") - AmqpConstants.StartOfEpoch).TotalMilliseconds;
            }

            if ((sections & SectionFlag.DeliveryAnnotations) != 0)
            {
                message.DeliveryAnnotations.Map["delivery-annotation"] = "delivery annotation";
            }

            if ((sections & SectionFlag.MessageAnnotations) != 0)
            {
                message.MessageAnnotations.Map["message-annotation"] = "message annotation";
            }

            if ((sections & SectionFlag.Properties) != 0)
            {
                message.Properties.MessageId = "message 1";
            }

            if ((sections & SectionFlag.ApplicationProperties) != 0)
            {
                message.ApplicationProperties.Map["message-prop"] = "message property";
            }

            if ((sections & SectionFlag.Footer) != 0)
            {
                message.Footer.Map["footer-prop"] = "footer";
            }
        }

        static void RunSerializationTest(AmqpMessage message)
        {
            AmqpMessage deserialized = AmqpMessage.CreateAmqpStreamMessage(new BufferListStream(ReadMessagePayLoad(message, 89)));
            ValidateMessage(message, deserialized);
        }

        static void ValidateMessage(AmqpMessage original, AmqpMessage deserialized)
        {
            Assert.AreEqual(original.Sections, deserialized.Sections);

            if ((original.Sections & SectionFlag.Header) != 0)
            {
                Assert.AreEqual(original.Header.Priority.Value, deserialized.Header.Priority.Value);
                Assert.AreEqual(original.Header.Ttl.Value, deserialized.Header.Ttl.Value);
            }

            if ((original.Sections & SectionFlag.DeliveryAnnotations) != 0)
            {
                Assert.AreEqual(original.DeliveryAnnotations.Map.Count(), deserialized.DeliveryAnnotations.Map.Count());
                foreach (var pair in original.DeliveryAnnotations.Map)
                {
                    Assert.AreEqual(original.DeliveryAnnotations.Map[pair.Key], deserialized.DeliveryAnnotations.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.MessageAnnotations) != 0)
            {
                Assert.AreEqual(original.MessageAnnotations.Map.Count(), deserialized.MessageAnnotations.Map.Count());
                foreach (var pair in original.MessageAnnotations.Map)
                {
                    Assert.AreEqual(original.MessageAnnotations.Map[pair.Key], deserialized.MessageAnnotations.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.Properties) != 0)
            {
                Assert.AreEqual(original.Properties.MessageId.ToString(), deserialized.Properties.MessageId.ToString());
            }

            if ((original.Sections & SectionFlag.ApplicationProperties) != 0)
            {
                Assert.AreEqual(original.ApplicationProperties.Map.Count(), deserialized.ApplicationProperties.Map.Count());
                foreach (var pair in original.ApplicationProperties.Map)
                {
                    Assert.AreEqual(original.ApplicationProperties.Map[pair.Key], deserialized.ApplicationProperties.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.Footer) != 0)
            {
                Assert.AreEqual(original.Footer.Map.Count(), deserialized.Footer.Map.Count());
                foreach (var pair in original.Footer.Map)
                {
                    Assert.AreEqual(original.Footer.Map[pair.Key], deserialized.Footer.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.Data) != 0)
            {
                try
                {
                    Assert.AreEqual(original.DataBody.Count(), deserialized.DataBody.Count());
                    using (IEnumerator<Data> enumerator1 = original.DataBody.GetEnumerator())
                    using (IEnumerator<Data> enumerator2 = deserialized.DataBody.GetEnumerator())
                    {
                        while (enumerator1.MoveNext() && enumerator2.MoveNext())
                        {
                            ArraySegment<byte> data1 = (ArraySegment<byte>)enumerator1.Current.Value;
                            ArraySegment<byte> data2 = (ArraySegment<byte>)enumerator1.Current.Value;
                            Assert.AreEqual(data1.Count, data2.Count);
                        }
                    }
                }
                catch (InvalidOperationException)
                {
                    // some messages do not support DataBody property
                    Stream bodyStream1 = original.BodyStream;
                    Stream bodyStream2 = deserialized.BodyStream;
                    Assert.AreEqual(bodyStream1.Length, bodyStream2.Length);
                }
            }

            if ((original.Sections & SectionFlag.AmqpValue) != 0)
            {
                Assert.AreEqual(original.ValueBody.Value, deserialized.ValueBody.Value);
            }

            if ((original.Sections & SectionFlag.AmqpSequence) != 0)
            {
                Assert.AreEqual(original.SequenceBody.Count(), deserialized.SequenceBody.Count());
                using(IEnumerator<AmqpSequence> enumerator1 = original.SequenceBody.GetEnumerator())
                using(IEnumerator<AmqpSequence> enumerator2 = deserialized.SequenceBody.GetEnumerator())
                {
                    while (enumerator1.MoveNext() && enumerator2.MoveNext())
                    {
                        Assert.AreEqual(enumerator1.Current.List.Count, enumerator2.Current.List.Count);
                        for(int i = 0; i < enumerator1.Current.List.Count; i++)
                        {
                            Assert.AreEqual(enumerator1.Current.List[i], enumerator2.Current.List[i]); 
                        }
                    }
                }
            }
        }
    }
}
