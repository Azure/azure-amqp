namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpMessageTests
    {
        [Fact]
        public void AmqpMessageReceiveResendTest()
        {
            var message = AmqpMessage.Create(new AmqpValue { Value = "Hello, AMQP!" });
            message.MessageAnnotations.Map["key"] = "old";

            // send the message and receive it on remote side
            ByteBuffer payload = message.GetBuffer();
            var outMessage = AmqpMessage.CreateBufferMessage(payload).Clone();
            //explicitly assign
            outMessage.Header.Priority = 99;
            outMessage.DeliveryAnnotations.Map["key"] = "da-update";
            outMessage.MessageAnnotations.Map["key"] = "update";

            // send
            var payload2 = outMessage.GetBuffer();
            var value = (string)outMessage.MessageAnnotations.Map["key"];
            Assert.Equal("update", value);

            // receive
            var receivedMessage = AmqpMessage.CreateBufferMessage(payload2);
            Assert.Equal(99, receivedMessage.Header.Priority.Value);
            value = (string)outMessage.DeliveryAnnotations.Map["key"];
            Assert.Equal("da-update", value);
            value = (string)outMessage.MessageAnnotations.Map["key"];
            Assert.Equal("update", value);
        }

        [Fact]
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
            // buffer message - received on broker side, clone for edit
            AmqpMessage message2 = CreateMessage(message, 71).Clone();
            AddSection(message2, SectionFlag.Header | SectionFlag.DeliveryAnnotations);
            message2.MessageAnnotations.Map["delivery-count"] = 5;
            // buffer message - received on client side
            AmqpMessage message3 = CreateMessage(message2, 73);
            // update the original message to match the updated message
            AddSection(message, SectionFlag.Header | SectionFlag.DeliveryAnnotations);
            message.MessageAnnotations.Map["delivery-count"] = 5;
            ValidateMessage(message, message3);
        }

        [Fact]
        public void AmqpMessageSerializedSizeTest()
        {
            var messages = new AmqpMessage[]
            {
                AmqpMessage.Create(new Data() { Value = new ArraySegment<byte>(new byte[60]) }),
                AmqpMessage.Create(new MemoryStream(new byte[1000]), true)
            };

            foreach (var message in messages)
            {
                long size = message.Serialize(true);
                Assert.True(size > 0);

                message.Properties.MessageId = Guid.NewGuid();
                long size2 = message.Serialize(true);
                Assert.True(size2 > size);

                message.MessageAnnotations.Map["property"] = "v1";
                long size3 = message.Serialize(true);
                Assert.True(size3 > size2);

                var message2 = AmqpMessage.CreateBufferMessage(message.GetBuffer());
                Assert.Equal("v1", message2.MessageAnnotations.Map["property"]);

                message.Properties.MessageId = "12345";
                message.MessageAnnotations.Map["property"] = "v2";
                message.Serialize(true);
                var message3 = AmqpMessage.CreateBufferMessage(message.GetBuffer());
                Assert.Equal((MessageId)"12345", message3.Properties.MessageId);
                Assert.Equal("v2", message3.MessageAnnotations.Map["property"]);
            }
        }

        static ByteBuffer[] ReadMessagePayLoad(AmqpMessage message, int payloadSize)
        {
            List<ByteBuffer> buffers = new List<ByteBuffer>();
            bool more = true;
            while (more)
            {
               ByteBuffer buffer = message.GetPayload(payloadSize, out more);
                if (buffer != null)
                {
                    buffers.Add(buffer);
                    message.CompletePayload(buffer.Length);
                }
            }

            return buffers.ToArray();
        }

        [Fact]
        public void AmqpMessageStreamTest()
        {
            AmqpMessage message = AmqpMessage.Create(new MemoryStream(new byte[12]), true);
            Assert.Equal(12, message.BodyStream.Length);

            AmqpMessage message2 = AmqpMessage.Create(new MemoryStream(new byte[12]), false);
            Assert.Equal(12, message2.BodyStream.Length);
        }

        static void AddSection(AmqpMessage message, SectionFlag sections)
        {
            if ((sections & SectionFlag.Header) != 0)
            {
                message.Header.Priority = 10;
                message.Header.Ttl = (uint)TimeSpan.FromDays(10).TotalMilliseconds;
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
            AmqpMessage deserialized = AmqpMessage.CreateBufferMessage(message.GetBuffer());
            ValidateMessage(message, deserialized);
        }

        static AmqpMessage CreateMessage(AmqpMessage source, int segmentSize)
        {
            ByteBuffer[] buffers = ReadMessagePayLoad(source, segmentSize);
            AmqpMessage message = AmqpMessage.CreateReceivedMessage();
            for (int i = 0; i < buffers.Length; i++)
            {
                message.AddPayload(buffers[i], i == buffers.Length - 1);
                buffers[i].Dispose();
            }

            return message;
        }

        static void ValidateMessage(AmqpMessage original, AmqpMessage deserialized)
        {
            Assert.Equal(original.Sections, deserialized.Sections);

            if ((original.Sections & SectionFlag.Header) != 0)
            {
                Assert.Equal(original.Header.Priority.Value, deserialized.Header.Priority.Value);
                Assert.Equal(original.Header.Ttl.Value, deserialized.Header.Ttl.Value);
            }

            if ((original.Sections & SectionFlag.DeliveryAnnotations) != 0)
            {
                Assert.Equal(original.DeliveryAnnotations.Map.Count(), deserialized.DeliveryAnnotations.Map.Count());
                foreach (var pair in original.DeliveryAnnotations.Map)
                {
                    Assert.Equal(original.DeliveryAnnotations.Map[pair.Key], deserialized.DeliveryAnnotations.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.MessageAnnotations) != 0)
            {
                Assert.Equal(original.MessageAnnotations.Map.Count(), deserialized.MessageAnnotations.Map.Count());
                foreach (var pair in original.MessageAnnotations.Map)
                {
                    Assert.Equal(original.MessageAnnotations.Map[pair.Key], deserialized.MessageAnnotations.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.Properties) != 0)
            {
                Assert.Equal(original.Properties.MessageId.ToString(), deserialized.Properties.MessageId.ToString());
            }

            if ((original.Sections & SectionFlag.ApplicationProperties) != 0)
            {
                Assert.Equal(original.ApplicationProperties.Map.Count(), deserialized.ApplicationProperties.Map.Count());
                foreach (var pair in original.ApplicationProperties.Map)
                {
                    Assert.Equal(original.ApplicationProperties.Map[pair.Key], deserialized.ApplicationProperties.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.Footer) != 0)
            {
                Assert.Equal(original.Footer.Map.Count(), deserialized.Footer.Map.Count());
                foreach (var pair in original.Footer.Map)
                {
                    Assert.Equal(original.Footer.Map[pair.Key], deserialized.Footer.Map[pair.Key]);
                }
            }

            if ((original.Sections & SectionFlag.Data) != 0)
            {
                try
                {
                    Assert.Equal(original.DataBody.Count(), deserialized.DataBody.Count());
                    using (IEnumerator<Data> enumerator1 = original.DataBody.GetEnumerator())
                    using (IEnumerator<Data> enumerator2 = deserialized.DataBody.GetEnumerator())
                    {
                        while (enumerator1.MoveNext() && enumerator2.MoveNext())
                        {
                            ArraySegment<byte> data1 = (ArraySegment<byte>)enumerator1.Current.Value;
                            ArraySegment<byte> data2 = (ArraySegment<byte>)enumerator1.Current.Value;
                            Assert.Equal(data1.Count, data2.Count);
                        }
                    }
                }
                catch (InvalidOperationException)
                {
                    // some messages do not support DataBody property
                    Stream bodyStream1 = original.BodyStream;
                    Stream bodyStream2 = deserialized.BodyStream;
                    Assert.Equal(bodyStream1.Length, bodyStream2.Length);
                }
            }

            if ((original.Sections & SectionFlag.AmqpValue) != 0)
            {
                Assert.Equal(original.ValueBody.Value, deserialized.ValueBody.Value);
            }

            if ((original.Sections & SectionFlag.AmqpSequence) != 0)
            {
                Assert.Equal(original.SequenceBody.Count(), deserialized.SequenceBody.Count());
                using(IEnumerator<AmqpSequence> enumerator1 = original.SequenceBody.GetEnumerator())
                using(IEnumerator<AmqpSequence> enumerator2 = deserialized.SequenceBody.GetEnumerator())
                {
                    while (enumerator1.MoveNext() && enumerator2.MoveNext())
                    {
                        Assert.Equal(enumerator1.Current.List.Count, enumerator2.Current.List.Count);
                        for(int i = 0; i < enumerator1.Current.List.Count; i++)
                        {
                            Assert.Equal(enumerator1.Current.List[i], enumerator2.Current.List[i]); 
                        }
                    }
                }
            }
        }
    }
}
