// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Threading;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Transaction;
    using TestAmqpBroker;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpLinkTests : IClassFixture<TestAmqpBrokerFixture>
    {
        Uri addressUri;
        TestAmqpBroker broker;

        public AmqpLinkTests(TestAmqpBrokerFixture testAmqpBrokerFixture)
        {
            addressUri = TestAmqpBrokerFixture.Address;
            broker = testAmqpBrokerFixture.Broker;
        }

        [Fact]
        public void AmqpLinkSyncSendReceiveTest()
        {
            const int messageCount = 10;
            string queue = "AmqpLinkSyncSendReceiveTestQueue";
            broker.AddQueue(queue);
            
            this.SendReceive(queue, messageCount, true, true, false);
        }
        
        [Fact]
        public void AmqpLinkAsyncSendReceiveTest()
        {
            const int messageCount = 10;
            string queue = "AmqpLinkAsyncSendReceiveTestQueue";
            broker.AddQueue(queue);

            this.SendReceive(queue, messageCount);
        }

        [Fact]
        public void AmqpLinkSettleOnSendTest()
        {
            const int messageCount = 30;
            string queue = "AmqpSettleOnSendTestQueue";
            broker.AddQueue(queue);

            this.SendReceive(
                queue,
                messageCount,
                true,
                true,
                true,
                (int)AmqpConstants.DefaultMaxFrameSize,
                0,
                (s) => { s.IncomingWindow = 8; s.OutgoingWindow = 12; },
                (s) => { s.SettleType = SettleMode.SettleOnSend; },
                (s) => { s.TotalLinkCredit = 26; s.SettleType = SettleMode.SettleOnSend; });
        }

        [Fact]
        public void AmqpLinkSettleOnReceiveTest()
        {
            const int messageCount = 30;
            string queue = "AmqpSettleOnReceiveTestQueue";
            broker.AddQueue(queue);

            this.SendReceive(
                queue,
                messageCount,
                true,
                true,
                true,
                (int)AmqpConstants.DefaultMaxFrameSize,
                0,
                (s) => { s.IncomingWindow = 8; s.OutgoingWindow = 12; },
                (s) => { s.SettleType = SettleMode.SettleOnReceive; },
                (s) => { s.TotalLinkCredit = 26; s.SettleType = SettleMode.SettleOnReceive; });
        }

        [Fact]
        public void AmqpLinkSettleOnDisposeTest()
        {
            const int messageCount = 30;
            string queue = "AmqpSettleOnDisposeTestQueue";
            broker.AddQueue(queue);

            this.SendReceive(
                queue,
                messageCount,
                true,
                true,
                true,
                (int)AmqpConstants.DefaultMaxFrameSize,
                0,
                (s) => { s.IncomingWindow = 8; s.OutgoingWindow = 12; },
                (s) => { s.SettleType = SettleMode.SettleOnDispose; },
                (s) => { s.TotalLinkCredit = 26; s.SettleType = SettleMode.SettleOnDispose; });
        }

        [Fact]
        public void AmqpLinkBestEffortSendTest()
        {
            const int messageCount = 10;
            string queue = "AmqpLinkBestEffortSendTest";
            broker.AddQueue(queue);

            // This test differs from SettleOnSend in that the initial link/session credit is
            // enough to transfer all messages, so it tests whether the publisher publishes all
            // messages even when the link is closed right after the last send.
            this.SendReceive(
                queue,
                messageCount,
                true,
                true,
                true,
                (int)AmqpConstants.DefaultMaxFrameSize,
                64,
                null,
                (s) => { s.SettleType = SettleMode.SettleOnSend; },
                (s) => { s.TotalLinkCredit = 50; s.SettleType = SettleMode.SettleOnSend; });
        }

        [Fact]
        public void AmqpAsyncAndOrderTest()
        {
            const int messageCount = 100;
            string queue = "AmqpAsyncAndOrderTestQueue";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            string messageBody = "Hello AMQP!";
            Exception lastException = null;

            Action sendAction = () =>
            {
                ManualResetEvent doneEvent = new ManualResetEvent(false);
                int sentCount = 0;
                Action<Delivery> dispositionHandler = (d) => { if (Interlocked.Increment(ref sentCount) == messageCount) doneEvent.Set(); };

                try
                {
                    SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnReceive));
                    sLink.RegisterDispositionListener(dispositionHandler);
                    sLink.Open();

                    for (int i = 0; i < messageCount; ++i)
                    {
                        AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = messageBody });
                        message.Properties.MessageId = (ulong)i;
                        message.Batchable = true;
                        sLink.SendMessageNoWait(message, new ArraySegment<byte>(BitConverter.GetBytes(i)), new ArraySegment<byte>());
                    }

                    if (!doneEvent.WaitOne(60 * 1000))
                    {
                        throw new TimeoutException("Send did not complete in time");
                    }

                    sLink.Close();
                }
                catch (Exception exp)
                {
                    lastException = exp;
                }
            };

            Action receiveAction = () =>
            {
                ManualResetEvent doneEvent = new ManualResetEvent(false);
                int receiveCount = 0;
                Action<AmqpMessage> messageHandler = (m) =>
                {
                    if (m.Properties.MessageId.ToString() != receiveCount.ToString())
                    {
                        lastException = new Exception(string.Format("received message id is not in order, expect: {0}, actual:{1}", receiveCount, m.Properties.MessageId));
                    }
                    else
                    {
                        m.Link.DisposeDelivery(m, true, AmqpConstants.AcceptedOutcome);
                    }
                    m.Dispose();
                    ++receiveCount;
                    if (lastException != null || receiveCount == messageCount)
                    {
                        doneEvent.Set();
                    }
                };

                try
                {
                    ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnSend, 100));
                    rLink.RegisterMessageListener(messageHandler);
                    rLink.Open();

                    if (!doneEvent.WaitOne(60 * 1000))
                    {
                        throw new TimeoutException("receive did not complete in time");
                    }

                    rLink.Close();
                }
                catch (Exception exp)
                {
                    lastException = exp;
                }
            };

            Task.WhenAll(Task.Run(sendAction), Task.Run(receiveAction)).Wait();

            Assert.True(lastException == null, string.Format("Failed. Last exception {0}", lastException == null ? string.Empty : lastException.ToString()));

            session.Close();
            connection.Close();
        }

        [Fact]
        public async Task AmqpLinkDrainTest()
        {
            string queue = "AmqpLinkDrainTest";
            broker.AddQueue(queue);

            var factory = new AmqpConnectionFactory();
            var connection = await factory.OpenConnectionAsync(TestAmqpBrokerFixture.Address, TimeSpan.FromSeconds(10));
            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            await session.OpenAsync(CancellationToken.None);

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            await rLink.OpenAsync(CancellationToken.None);

            await rLink.DrainAsyc(CancellationToken.None);
            Assert.Equal(0u, rLink.LinkCredit);

            await connection.CloseAsync(CancellationToken.None);
        }

        [Fact]
        public void AmqpMessageTest()
        {
            string queue = "AmqpMessageTest";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            string data = new string('a', 128 * 1024);
            AmqpMessage message = AmqpMessage.Create(new Data() { Value = new ArraySegment<byte>(System.Text.Encoding.UTF8.GetBytes(data)) });
            message.Header.Priority = 1;
            message.DeliveryAnnotations.Map.Add("test", "da");
            message.MessageAnnotations.Map.Add("test", "ma");
            message.Properties.MessageId = "12345";
            message.ApplicationProperties.Map.Add("transform", "sepia");
            message.ApplicationProperties.Map.Add("image_name", "carpark");
            message.ApplicationProperties.Map.Add("image_type", "jpg");
            message.ApplicationProperties.Map.Add("client_name", "rob-jms");
            message.Footer.Map.Add("signature", "foo");
            sLink.EndSendMessage(sLink.BeginSendMessage(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10), null, null));

            sLink.Close();

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            rLink.Open();
            bool hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(60), null, null), out message);
            Assert.True(hasMessage);
            Assert.NotNull(message);
            Assert.NotNull(message.ApplicationProperties);
            Assert.Equal(SectionFlag.Data, message.BodyType);
            ArraySegment<byte> bytes = (ArraySegment<byte>)message.DataBody.First().Value;
            Assert.Equal(data, System.Text.Encoding.UTF8.GetString(bytes.Array, bytes.Offset, bytes.Count));

            message.Dispose();
            rLink.Close();
            connection.Close();
        }

        [Fact]
        public void AmqpMessageEmptyTest()
        {
            string queue = "AmqpMessageEmptyTest";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            try
            {
                var message = AmqpMessage.Create();
                sLink.SendMessageAsync(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary,
                    TimeSpan.FromSeconds(30)).GetAwaiter().GetResult();
            }
            catch (InvalidOperationException)
            {
            }

            try
            {
                var message = AmqpMessage.Create(new Data[0]);
                sLink.SendMessageAsync(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary,
                    TimeSpan.FromSeconds(30)).GetAwaiter().GetResult();
            }
            catch (InvalidOperationException)
            {
            }

            connection.Close();
        }

        [Fact]
        public void AmqpDynamicNodeTest()
        {
            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, null, SettleMode.SettleOnReceive, 1, true));
            rLink.Open();

            string dynamicAddress = rLink.Settings.Address().ToString();
            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, dynamicAddress, SettleMode.SettleOnReceive));
            sLink.Open();

            string messageBody = "Hello AMQP!";
            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = messageBody });
            message.Batchable = false;
            Outcome outcome = sLink.EndSendMessage(sLink.BeginSendMessage(message, new ArraySegment<byte>(new byte[2]), new ArraySegment<byte>(), TimeSpan.FromSeconds(5), null, null));
            Assert.True(outcome.DescriptorCode == Accepted.Code, "message is not accepted.");

            AmqpMessage message2;
            bool hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(10), null, null), out message2);
            Assert.True(hasMessage, "receive should return true");
            Assert.True(message2 != null, "The received message cannot be null.");
            Assert.True(message.ValueBody != null, "the message body should have a valid value");
            Assert.True(message.ValueBody.Value.Equals(messageBody), "Received a different message.");
            rLink.AcceptMessage(message, true, false);
            message.Dispose();

            sLink.Close();  // close this first since the dynamic node is deleted when rLink is closed
            rLink.Close();
            session.Close();
            connection.Close();
        }

        [Fact]
        public void AmqpMessageFragmentationTest()
        {
            const int sendMaxFrameSize = 512;
            const int receiveMaxFrameSize = 1100;
            const int bodySize = 5000;
            const int messageCount = 10;
            string queue = "AmqpMessageFragmentationTestQueue";
            broker.AddQueue(queue);

            this.SendReceive(queue, messageCount, true, false, true, sendMaxFrameSize, bodySize);
            this.SendReceive(queue, messageCount, false, true, true, receiveMaxFrameSize, bodySize);
        }

        [Fact]
        public void AmqpMultipleSettleModeLinksTest()
        {
            const int messageCount = 29;
            string queue = "AmqpMultipleSettleModeLinksTestQueue";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings() { IncomingWindow = 9, OutgoingWindow = 11 });
            session.Open();

            Exception lastException = null;
            Action<SettleMode> sendAction = (mode) =>
            {
                ManualResetEvent doneEvent = new ManualResetEvent(false);
                int sentCount = 0;
                Action<Delivery> dispositionHandler = (d) =>
                    {
                        if (d.State.DescriptorCode != Accepted.Code)
                        {
                            doneEvent.Set();
                            return;
                        }

                        if (!d.Settled)
                        {
                            d.Link.DisposeDelivery(d, true, d.State);
                        }

                        if (Interlocked.Increment(ref sentCount) == messageCount)
                        {
                            doneEvent.Set();
                        }
                    };

                try
                {
                    SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, mode));
                    sLink.RegisterDispositionListener(dispositionHandler);
                    sLink.Open();

                    string messageBody = "Hello AMQP!";
                    for (int i = 0; i < messageCount; ++i)
                    {
                        AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = messageBody });
                        message.Properties.MessageId = (ulong)i;
                        message.Batchable = true;
                        sLink.SendMessageNoWait(message, new ArraySegment<byte>(BitConverter.GetBytes(i)), new ArraySegment<byte>());
                    }

                    if (!doneEvent.WaitOne(10 * 1000))
                    {
                        throw new TimeoutException("Send did not complete in time");
                    }

                    if (sentCount < messageCount)
                    {
                        throw new Exception("sent count is less than the totoal count");
                    }

                    sLink.Close();
                }
                catch (Exception exp)
                {
                    lastException = exp;
                }
            };

            SettleMode[] modes = new SettleMode[] { SettleMode.SettleOnSend, SettleMode.SettleOnReceive, SettleMode.SettleOnDispose };
            Task.WhenAll(modes.Select(m => Task.Run(() => sendAction(m)))).Wait();

            Assert.True(lastException == null, string.Format("Failed. Last exception {0}", lastException == null ? string.Empty : lastException.ToString()));

            session.Close();
            connection.Close();
        }

        [Fact]
        public void NonPrefetchConcurrentReceiveTest()
        {
            string queue = "NonPrefetchConcurrentReceiveTest";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnSend, 0));
            rLink.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            bool done = false;
            int count = 0;
            Task sendTask = Task.Run(async () =>
            {
                while (!done)
                {
                    AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "Test" });
                    Outcome outcome = await sLink.SendMessageAsync(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, CancellationToken.None);
                    Assert.Equal(outcome.DescriptorCode, Accepted.Code);
                }
            });

            Task.WaitAll(Enumerable.Range(0, 30).Select(i =>
            {
                return Task.Run(async () =>
                {
                    while (!done)
                    {
                        var msg = await rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(10));
                        Assert.NotNull(msg);
                        rLink.AcceptMessage(msg, false);
                        if (Interlocked.Increment(ref count) > 10000)
                        {
                            break;
                        }
                    }
                });
            }).ToArray());

            done = true;
            sendTask.Wait();

            connection.Close();
        }

        [Fact]
        public void AmqpSequenceNumberWrapAroundTest()
        {
            const int messageCount = 24;
            string queue = "AmqpSequenceNumberWrapAroundTestQueue";
            broker.AddQueue(queue);

            this.SendReceive(
                queue,
                messageCount,
                true,
                false,
                true,
                (int)AmqpConstants.DefaultMaxFrameSize,
                0,
                (s) => { s.NextOutgoingId = uint.MaxValue - 3; s.InitialDeliveryId = uint.MaxValue - 7; s.IncomingWindow = 8; s.OutgoingWindow = 12; },
                null,
                (s) => { s.InitialDeliveryCount = uint.MaxValue - 2; s.TotalLinkCredit = 18; });
        }

        [Fact]
        public void AmqpPipeLineModeUsingAPITest()
        {
            string queue = "AmqpPipeLineModeUsingAPITestQueue";
            broker.AddQueue(queue);

            List<Action> results = new List<Action>();
            Action<IAsyncResult, Action<IAsyncResult>> addResult = (r, a) => { results.Add(() => { a(r); }); };

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, 1024);
            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnReceive));
            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "Hello AMQP!" });

            addResult(connection.BeginOpen(TimeSpan.FromSeconds(10), null, null), connection.EndOpen);
            addResult(session.BeginOpen(TimeSpan.FromSeconds(10), null, null), session.EndOpen);
            addResult(sLink.BeginOpen(TimeSpan.FromSeconds(10), null, null), sLink.EndOpen);
            addResult(sLink.BeginSendMessage(message, new ArraySegment<byte>(new byte[] { 0 }, 0, 1),
                new ArraySegment<byte>(), TimeSpan.FromSeconds(10), null, null), (a) => { sLink.EndSendMessage(a); });

            foreach (var item in results) item();
            results.Clear();

            //addResult(sLink.BeginClose(TimeSpan.FromSeconds(10), null, null), sLink.EndClose);
            //addResult(session.BeginClose(TimeSpan.FromSeconds(10), null, null), session.EndClose);
            addResult(connection.BeginClose(TimeSpan.FromSeconds(10), null, null), connection.EndClose);
            foreach (var item in results) item();
        }

        [Fact]
        public void AmqpOperationTimeoutTest()
        {
            string queue = "AmqpOperationTimeoutTest";
            broker.AddQueue(queue);

            Func<AmqpObject, TimeSpan> func = obj => (TimeSpan)obj.GetType().GetProperty("OperationTimeout", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(obj);
            TimeSpan timeout = TimeSpan.FromSeconds(28);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, 1024);
            connection.Settings.OperationTimeout = timeout;
            Assert.Equal(timeout, func(connection));

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            Assert.Equal(timeout, func(session));

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnReceive));
            Assert.Equal(timeout, func(sLink));

            sLink.Settings.OperationTimeout = TimeSpan.MaxValue;
            Assert.Equal(TimeSpan.MaxValue, func(sLink));

            connection.Close();
        }

#if !WINDOWS_UWP
        [Fact]
        public void AmqpTransactionTest()
        {
            const int messageCount = 6;
            string queue = "AmqpTransactionCommitTest";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, 10000);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            Controller txController = new Controller(session, TimeSpan.FromSeconds(10));
            txController.Open();

            ArraySegment<byte> txnId = txController.DeclareAsync().Result;

            SendingAmqpLink sendLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnReceive));
            sendLink.Open();

            // send message
            for (int i = 0; i < messageCount; ++i)
            {
                ArraySegment<byte> tag = new ArraySegment<byte>(BitConverter.GetBytes(i));
                AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "Hello AMQP!" });
                message.Properties.MessageId = (ulong)i;
                Outcome outcome = sendLink.EndSendMessage(sendLink.BeginSendMessage(message, tag, txnId, TimeSpan.FromSeconds(10), null, null));
                Assert.True(outcome.DescriptorCode == Accepted.Code, "message is not accepted.");
            }

            // rollback txn
            txController.DischargeAsync(txnId, true).Wait(TimeSpan.FromSeconds(10));

            txnId = txController.DeclareAsync().Result;

            // send message again
            for (int i = 0; i < messageCount; ++i)
            {
                ArraySegment<byte> tag = new ArraySegment<byte>(BitConverter.GetBytes(i));
                AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "Hello AMQP!" });
                message.Properties.MessageId = (ulong)i;
                Outcome outcome = sendLink.EndSendMessage(sendLink.BeginSendMessage(message, tag, txnId, TimeSpan.FromSeconds(10), null, null));
                Assert.True(outcome.DescriptorCode == Accepted.Code, "message is not accepted.");
            }

            // commit txn
            txController.DischargeAsync(txnId, false).Wait(TimeSpan.FromSeconds(10));

            ReceivingAmqpLink receiveLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 20));
            receiveLink.Open();

            TransactionalState txnState = new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome };
            txnState.TxnId = txController.DeclareAsync().Result;

            // receive message
            AmqpMessage[] messages = new AmqpMessage[messageCount];
            for (int i = 0; i < messageCount; ++i)
            {
                receiveLink.EndReceiveMessage(receiveLink.BeginReceiveMessage(TimeSpan.FromSeconds(10), null, null), out messages[i]);
                receiveLink.DisposeMessage(messages[i], txnState, false, true);
                messages[i].Dispose();
            }
            receiveLink.Session.Flush();    // force dispositions out before discharge frames

            // rollback txn
            txController.DischargeAsync(txnState.TxnId, true).Wait(TimeSpan.FromSeconds(10));

            txnState.TxnId = txController.DeclareAsync().Result;

            // complete message again
            for (int i = 0; i < messageCount; ++i)
            {
                receiveLink.DisposeMessage(messages[i], txnState, false, true);
            }
            receiveLink.Session.Flush();

            // commit txn
            txController.DischargeAsync(txnState.TxnId, false).Wait(TimeSpan.FromSeconds(10));

            txController.Close(TimeSpan.FromSeconds(5));
            sendLink.Close();
            session.Close();
            connection.Close();
        }
#endif

        [Fact]
        public void AmqpDynamicLinkCreditTest()
        {
            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            // do not open the link as messages are injected locally
            var settings = AmqpUtils.GetLinkSettings(false, "dummy", SettleMode.SettleOnDispose, 10);
            var link = new ReceivingAmqpLink(session, settings);
            settings.AutoSendFlow = false;
            Assert.Equal(10u, link.LinkCredit);

            FieldInfo bufferedCredit = typeof(AmqpLink).GetField("bufferedCredit", BindingFlags.NonPublic | BindingFlags.Instance);

            int receiveCount = 0;
            Action<AmqpMessage> onMessage = (o) =>
            {
                ++receiveCount;
            };

            link.RegisterMessageListener(onMessage);

            Action<int, int> sendMessages = (n, d) =>
            {
                for (int k = 0; k < n; k++)
                {
                    ByteBuffer buffer = new ByteBuffer(1024, true);
                    Frm(buffer, 0, Cmd(Transfer.Code, 0u, (uint)d++, AmqpConstants.EmptyBinary, 0u, true), new ArraySegment<byte>(new byte[] { 0x00, 0x53, 0x77, 0x44 }));
                    link.ProcessFrame(Frm(buffer));
                }
            };

            // run 4 messages through the link
            sendMessages(4, 0);
            Assert.Equal(6u, link.LinkCredit);

            // reduce credit
            link.SetTotalLinkCredit(6, true);
            Assert.Equal(2u, link.LinkCredit);
            Assert.Equal(4u, bufferedCredit.GetValue(link));

            // consume partial buffered credit
            sendMessages(2, 4);
            Assert.Equal(2u, link.LinkCredit);
            Assert.Equal(2u, bufferedCredit.GetValue(link));

            // increase credit
            link.SetTotalLinkCredit(7, true);
            Assert.Equal(3u, link.LinkCredit);
            Assert.Equal(1u, bufferedCredit.GetValue(link));

            // consume all credits
            sendMessages(4, 6);
            Assert.Equal(0u, link.LinkCredit);
            Assert.Equal(0u, bufferedCredit.GetValue(link));

            // reset credit to 10
            link.IssueCredit(10, false, AmqpConstants.NullBinary);
            Assert.Equal(10u, link.LinkCredit);
            Assert.Equal(0u, bufferedCredit.GetValue(link));

            // delayed udpate
            link.SetTotalLinkCredit(0, false);
            Assert.Equal(10u, link.LinkCredit);
            Assert.Equal(0u, bufferedCredit.GetValue(link));
            sendMessages(1, 10);
            Assert.Equal(0u, link.LinkCredit);
            Assert.Equal(9u, bufferedCredit.GetValue(link));

            link.SetTotalLinkCredit(3, true);
            Assert.Equal(3u, link.LinkCredit);
            Assert.Equal(6u, bufferedCredit.GetValue(link));

            // no flow control
            link.SetTotalLinkCredit(uint.MaxValue, true);
            Assert.Equal(uint.MaxValue, link.LinkCredit);
            Assert.Equal(0u, bufferedCredit.GetValue(link));

            sendMessages(10, 11);
            Assert.Equal(uint.MaxValue, link.LinkCredit);
            Assert.Equal(0u, bufferedCredit.GetValue(link));

            link.Abort();
            connection.Close();
        }

        [Fact]
        public void AmqpTransferWithFlowControlTest()
        {
            string entity = "AmqpTransferWithFlowControlTest";
            broker.AddQueue(entity);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sender = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, entity, SettleMode.SettleOnSend));
            sender.Open();
            for (int i = 0; i < 100; i++)
            {
                sender.SendMessageNoWait(AmqpMessage.Create(new AmqpValue() { Value = "hello" }), AmqpConstants.EmptyBinary, AmqpConstants.NullBinary);
            }

            ReceivingAmqpLink receiver = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, entity, SettleMode.SettleOnSend, 10));
            int messageReceived = 0;
            ManualResetEvent completed = new ManualResetEvent(false);
            receiver.RegisterMessageListener((m) =>
                {
                    try
                    {
                        receiver.DisposeDelivery(m, true, AmqpConstants.AcceptedOutcome);
                        if (messageReceived == 5)
                        {
                            receiver.SetTotalLinkCredit(20, false);
                        }
                        else if (messageReceived == 13)
                        {
                            receiver.SetTotalLinkCredit(0, true);
                            Task.Delay(200).Wait();
                            receiver.SetTotalLinkCredit(10, false);
                        }
                        else if (messageReceived == 33)
                        {
                            receiver.SetTotalLinkCredit(50, false);
                        }
                        else if (messageReceived == 49)
                        {
                            receiver.SetTotalLinkCredit(2, true);
                        }

                        if (++messageReceived == 100)
                        {
                            completed.Set();
                        }
                    }
                    catch (Exception exception)
                    {
                        Debug.WriteLine(exception.ToString());
                        completed.Set();
                    }
                });

            receiver.Open();

            completed.WaitOne(TimeSpan.FromSeconds(30));
            Assert.Equal(100, messageReceived);

            connection.Close();
        }

        [Fact]
        public void AmqpConnectionIdleTimeoutTest()
        {
            string queue = "AmqpConnectionIdleTimeoutTest";
            broker.AddQueue(queue);

            uint? localIdelTimeOut = 60 * 1000;
            uint? remoteIdleTimeOut = null;
            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null,
                (int)AmqpConstants.DefaultMaxFrameSize, localIdelTimeOut);
            Assert.Equal(localIdelTimeOut, connection.Settings.IdleTimeOut);
            connection.Opening += (c, o) => remoteIdleTimeOut = ((Open)o.Command).IdleTimeOut;
            connection.Open();
            connection.Close();

#if !WINDOWS_UWP
            Assert.NotNull(remoteIdleTimeOut);
            Assert.Equal(TestAmqpBroker.ConnectionIdleTimeOut / 2, remoteIdleTimeOut.Value);
#endif

            bool gotException = false;
            connection = AmqpUtils.CreateConnection(addressUri, null, false, null,
                (int)AmqpConstants.DefaultMaxFrameSize, 1 * 1000);
            try
            {
                connection.EndOpen(connection.BeginOpen(TimeSpan.FromMinutes(5), null, null));
            }
            catch (AmqpException exception)
            {
                gotException = true;
                Debug.WriteLine(exception.Message);
            }

            Assert.True(gotException);
        }

        [Fact]
        public void AmqpMessageCloneForResendTest()
        {
            string queue = "AmqpMessageCloneForResendTest" + Guid.NewGuid().ToString("N");
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnReceive));
            sLink.Open();

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            rLink.Open();

            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "test" });
            message.Header.Priority = 1;
            message.DeliveryAnnotations.Map.Add("test", "da");
            message.MessageAnnotations.Map.Add("test", "ma");
            message.Properties.MessageId = "12345";
            message.ApplicationProperties.Map.Add("transform", "sepia");
            message.ApplicationProperties.Map.Add("image_name", "carpark");
            message.ApplicationProperties.Map.Add("image_type", "jpg");
            message.ApplicationProperties.Map.Add("client_name", "rob-jms");
            message.Footer.Map.Add("signature", "foo");

            Outcome outcome = sLink.EndSendMessage(sLink.BeginSendMessage(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10), null, null));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            for (int i = 0; i < 10; i++)
            {
                bool hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(60), null, null), out message);
                Assert.True(hasMessage);
                Assert.NotNull(message);

                AmqpMessage newMessage = message.Clone();
                rLink.DisposeMessage(message, AmqpConstants.AcceptedOutcome, true, false);

                outcome = sLink.EndSendMessage(sLink.BeginSendMessage(newMessage, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10), null, null));
                Assert.Equal(Accepted.Code, outcome.DescriptorCode);
            }

            rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(60), null, null), out message);
            Assert.NotNull(message);
            rLink.DisposeMessage(message, AmqpConstants.AcceptedOutcome, true, false);

            connection.Close();
        }

        [Fact]
        public void AmqpPeekLockReleaseAcceptTest()
        {
            string queue = "AmqpPeekLockReleaseAcceptTest" + Guid.NewGuid().ToString("N");
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "test" });
            Outcome outcome = sLink.EndSendMessage(sLink.BeginSendMessage(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10), null, null));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            rLink.Open();

            bool hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(20), null, null), out message);
            Assert.True(hasMessage);
            Assert.NotNull(message);
            rLink.ReleaseMessage(message);

            hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(20), null, null), out message);
            Assert.True(hasMessage);
            Assert.NotNull(message);
            rLink.AcceptMessage(message, false);

            hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromMilliseconds(500), null, null), out message);
            Assert.False(hasMessage);
            Assert.Null(message);

            connection.Close();
        }

        [Fact]
        public void ReleaseMessageOnLinkCloseTest()
        {
            string queue = "ReleaseMessageOnLinkCloseTest" + Guid.NewGuid().ToString("N");
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            AmqpMessage message;
            for (int i = 0; i < 10; i++)
            {
                message = AmqpMessage.Create(new AmqpValue() { Value = "test" });
                Outcome outcome = sLink.EndSendMessage(sLink.BeginSendMessage(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10), null, null));
                Assert.Equal(Accepted.Code, outcome.DescriptorCode);
            }

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            rLink.Open();

            bool hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(20), null, null), out message);
            Assert.True(hasMessage);
            Assert.NotNull(message);

            rLink.Close();

            rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            rLink.Open();
            hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(20), null, null), out message);
            Assert.True(hasMessage);
            Assert.NotNull(message);
            rLink.AcceptMessage(message, false);

            connection.Close();
        }

        [Fact]
        public void AmqpLinkCreditMaxValueTest()
        {
            string queue = "AmqpLinkCreditMaxValueTest-" + Guid.NewGuid().ToString("N").Substring(6);
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, (int)AmqpConstants.DefaultMaxFrameSize);
            connection.Open();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "test" });
            Outcome outcome = sLink.EndSendMessage(sLink.BeginSendMessage(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10), null, null));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            var settings = AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 0);
            settings.TotalLinkCredit = uint.MaxValue;
            settings.AutoSendFlow = true;
            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, settings);
            rLink.Open();

            bool hasMessage = rLink.EndReceiveMessage(rLink.BeginReceiveMessage(TimeSpan.FromSeconds(20), null, null), out message);
            Assert.True(hasMessage);
            Assert.NotNull(message);
            outcome = rLink.DisposeMessageAsync(message.DeliveryTag, new Accepted(), false, TimeSpan.FromSeconds(15)).Result;
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            connection.Close();
        }

        [Fact]
        public async Task OpenSequentialConnectionsToFindRaceConditions()
        {
            // NOTE: Increment this number to make it more likely to hit race conditions.
            const int NumberOfRuns = 500;

            string queue = "OpenSequentialConnectionsToFindRaceConditions";
            broker.AddQueue(queue);

            var timeout = TimeSpan.FromSeconds(10);
            for (int i = 0; i < NumberOfRuns; i++)
            {
                Debug.WriteLine("Iteration " + i);
                AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, 1024);
                await connection.OpenAsync(timeout);

                var sessionSettings = new AmqpSessionSettings() { DispositionInterval = TimeSpan.FromMilliseconds(20) };
                AmqpSession session = connection.CreateSession(sessionSettings);
                await session.OpenAsync(timeout);
                var senderSettings = new AmqpLinkSettings
                {
                    LinkName = "sender",
                    Role = false,
                    Source = new Source(),
                    Target = new Target { Address = queue }
                };

                SendingAmqpLink sender = new SendingAmqpLink(session, senderSettings);
                await sender.OpenAsync(timeout);

                var message = AmqpMessage.Create(new AmqpValue { Value = "Hello, AMQP!" });
                Outcome outcome = await sender.SendMessageAsync(message, new ArraySegment<byte>(Guid.NewGuid().ToByteArray()), AmqpConstants.NullBinary, timeout);
                message.Dispose();

                var receiverSettings = new AmqpLinkSettings
                {
                    LinkName = "receiver",
                    Role = true,
                    Source = new Source { Address = queue },
                    Target = new Target(),
                    AutoSendFlow = true,
                    TotalLinkCredit = 10  // this setting seems to have a big impact on this test's failure rate.
                };

                var receiver = new ReceivingAmqpLink(session, receiverSettings);
                await receiver.OpenAsync(timeout);
                await Task.Yield();

                var message2 = await receiver.ReceiveMessageAsync(timeout);
                await Task.Yield();
                Assert.NotNull(message2);

                receiver.AcceptMessage(message2, false);
                message2.Dispose();

                await sender.CloseAsync(timeout);
                await receiver.CloseAsync(timeout);
                await session.CloseAsync(timeout);
                await connection.CloseAsync(timeout);
            }
        }

        [Fact]
        public async Task AmqpConnectionFactoryTest()
        {
            string queue = "AmqpConnectionFactoryTest-" + Guid.NewGuid().ToString("N").Substring(6);
            broker.AddQueue(queue);

            AmqpConnection connection = await AmqpConnection.Factory.OpenConnectionAsync(addressUri, TimeSpan.FromSeconds(20));

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            await session.OpenAsync(TimeSpan.FromSeconds(20));

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            await sLink.OpenAsync(TimeSpan.FromSeconds(20));

            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "AmqpConnectionFactoryTest" });
            Outcome outcome = await sLink.SendMessageAsync(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnDispose, 10));
            await rLink.OpenAsync(TimeSpan.FromSeconds(20));

            var receivedMessage = await rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(20));
            Assert.NotNull(receivedMessage);
            outcome = await rLink.DisposeMessageAsync(receivedMessage.DeliveryTag, new Accepted(), false, TimeSpan.FromSeconds(20));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            await connection.CloseAsync(TimeSpan.FromSeconds(20));
        }

        void SendReceive(
            string queue,
            int messageCount = 1,
            bool doSend = true,
            bool doReceive = true,
            bool sendAsync = true,
            int frameSize = (int)AmqpConstants.DefaultMaxFrameSize,
            int messageSize = 0,
            Action<AmqpSessionSettings> sessionSettingsUpdater = null,
            Action<AmqpLinkSettings> sendLinkSettingsUpdater = null,
            Action<AmqpLinkSettings> receiveLinkSettingsUpdater = null)
        {
            AmqpConnection connection = AmqpUtils.CreateConnection(addressUri, null, false, null, frameSize);
            connection.Open();

            AmqpSessionSettings sessionSettings = new AmqpSessionSettings();
            sessionSettings.DispositionInterval = TimeSpan.FromMilliseconds(100);
            if (sessionSettingsUpdater != null)
            {
                sessionSettingsUpdater(sessionSettings);
            }

            AmqpSession session = connection.CreateSession(sessionSettings);
            session.Open();

            string messageBody = messageSize > 0 ? new string('a', messageSize) : "Hello AMQP!";

            if (doSend)
            {
                AmqpLinkSettings linkSettings = AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnReceive);
                if (sendLinkSettingsUpdater != null)
                {
                    sendLinkSettingsUpdater(linkSettings);
                }

                SendingAmqpLink sLink = new SendingAmqpLink(session, linkSettings);
                sLink.Open();

                int sendCompleted = 0;
                ManualResetEvent sendDone = new ManualResetEvent(false);
                if (sendAsync)
                {
                    sLink.RegisterDispositionListener((d) =>
                        {
                            if (d.State.DescriptorCode != Accepted.Code)
                            {
                                sendDone.Set();
                                return;
                            }

                            if (!d.Settled)
                            {
                                d.Link.DisposeDelivery(d, true, d.State);
                            }

                            if (Interlocked.Increment(ref sendCompleted) == messageCount)
                            {
                                sendDone.Set();
                            }
                        });
                }

                for (int i = 0; i < messageCount; ++i)
                {
                    ArraySegment<byte> deliveryTag = new ArraySegment<byte>(BitConverter.GetBytes(i));
                    ArraySegment<byte> txnId = new ArraySegment<byte>();
                    AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = messageBody });
                    message.Properties.MessageId = (ulong)i;
                    if (sendAsync)
                    {
                        message.Batchable = true;
                        sLink.SendMessageNoWait(message, deliveryTag, txnId);
                    }
                    else
                    {
                        IAsyncResult result = sLink.BeginSendMessage(message, deliveryTag, txnId, TimeSpan.FromSeconds(5), null, null);
                        sLink.EndSendMessage(result);
                    }
                }

                if (sendAsync)
                {
                    Assert.True(sendDone.WaitOne(10 * 1000), "Send did not complete in time.");
                    Assert.True(sendCompleted >= messageCount, "Sent count is less than the totoal count.");
                }

                sLink.Close();
            }

            if (doReceive)
            {
                AmqpLinkSettings linkSettings = AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 10);
                if (receiveLinkSettingsUpdater != null)
                {
                    receiveLinkSettingsUpdater(linkSettings);
                }
                ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, linkSettings);
                rLink.Open();

                for (int i = 0; i < messageCount; ++i)
                {
                    if (rLink.LinkCredit == 0)
                    {
                        rLink.IssueCredit(100, false, new ArraySegment<byte>());
                    }

                    IAsyncResult result = rLink.BeginReceiveMessage(TimeSpan.FromSeconds(100), null, null);
                    AmqpMessage message;
                    bool hasMessage = rLink.EndReceiveMessage(result, out message);
                    Assert.True(hasMessage, "receive should return true");
                    Assert.True(message != null, "The received message cannot be null.");
                    Assert.True(message.ValueBody != null, "the message body should have a valid value");
                    Assert.True(message.ValueBody.Value.Equals(messageBody), "Received a different message.");
                    //Assert.True(message.Properties.MessageId.ToString() == i.ToString(), "message id must be the same");
                    message.Dispose();
                    rLink.AcceptMessage(message, true, true);
                }

                rLink.Close();
            }

            session.Close();
            connection.Close();
        }

        static DescribedType Cmd(object descriptor, params object[] values)
        {
            return new DescribedType(descriptor, new List<object>(values));
        }

        static Frame Frm(ByteBuffer buffer)
        {
            Frame frame = new Frame();
            frame.Decode(buffer);
            return frame;
        }

        static void Frm(ByteBuffer buffer, ushort channel, DescribedType command, ArraySegment<byte> payload = default(ArraySegment<byte>))
        {
            int size = 8 + AmqpEncoding.GetObjectEncodeSize(command) + payload.Count;
            AmqpBitConverter.WriteUInt(buffer, (uint)size);
            AmqpBitConverter.WriteUByte(buffer, 2);
            AmqpBitConverter.WriteUByte(buffer, 0);
            AmqpBitConverter.WriteUShort(buffer, channel);
            AmqpEncoding.EncodeObject(command, buffer);
            if (payload.Count > 0)
            {
                AmqpBitConverter.WriteBytes(buffer, payload.Array, payload.Offset, payload.Count);
            }
        }

        static void SendCommand(AmqpConnectionBase connection, ushort channel, DescribedType command, ArraySegment<byte> payload = default(ArraySegment<byte>))
        {
            ByteBuffer buffer = new ByteBuffer(1024, true);
            Frm(buffer, channel, command, payload);
            connection.SendBuffers(new ByteBuffer[] { buffer });
        }

        static void PipeLineSend(string host, int port, string queue, byte[] message)
        {
            byte[] bytes = new byte[4096];

            // construct the buffer
            ByteBuffer buffer = new ByteBuffer(bytes);
            AmqpCodec.EncodeSerializable(new ProtocolHeader(ProtocolId.Amqp, new AmqpVersion(1, 0, 0)), buffer);
            Frm(buffer, 0, Cmd(Open.Code, "my-container", "my-hostname", (uint)1000));
            Frm(buffer, 0, Cmd(Begin.Code, null, (uint)200, (uint)300, (uint)400));
            Frm(buffer, 0, Cmd(Attach.Code, queue, (uint)0, false, null, null, null, new Target() { Address = queue }));
            Frm(buffer, 0, Cmd(Transfer.Code, (uint)0, (uint)0, new ArraySegment<byte>(new byte[] { 0 }), (uint)0, true, false), new ArraySegment<byte>(message));
            Frm(buffer, 0, Cmd(Detach.Code, (uint)0));
            Frm(buffer, 0, Cmd(End.Code, new object[0]));
            Frm(buffer, 0, Cmd(Close.Code, new object[0]));

            Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            socket.Connect(host, port);
            socket.Send(buffer.Buffer, buffer.Offset, buffer.Length, SocketFlags.None);

            // receive the frames
            int count = Receive(socket, bytes, 0);
            buffer = new ByteBuffer(bytes, 0, count);
            Assert.True(Frm(buffer).Command.DescriptorCode == Open.Code, "Open not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == Begin.Code, "Begin not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == Attach.Code, "Attach not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == Flow.Code, "Flow not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == Disposition.Code, "Disposition not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == Detach.Code, "Detach not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == End.Code, "End not received");
            Assert.True(Frm(buffer).Command.DescriptorCode == Close.Code, "Close not received");

            socket.Dispose();
        }

        static void Send(Socket socket, byte[] buffer, int offset, int count)
        {
            while (count > 0)
            {
                int bytes = socket.Send(buffer, offset, count, SocketFlags.None);
                offset += bytes;
                count -= bytes;
            }
        }

        static int Receive(Socket socket, byte[] buffer, int offset)
        {
            int bytesRead = 0;
            do
            {
                int count = socket.Receive(buffer, buffer.Length - offset, SocketFlags.None);
                if (count == 0)
                {
                    break;
                }

                offset += count;
                bytesRead += count;
            }
            while (true);
            return bytesRead;
        }
    }
}
