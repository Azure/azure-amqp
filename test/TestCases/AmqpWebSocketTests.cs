// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;
    using TestAmqpBroker;

    [TestClass]
    public class AmqpWebSocketTests
    {
        TestAmqpBroker broker;

        [TestInitialize]
        public void TestInitialize()
        {
            broker = TestAmqpBrokerFixture.Broker;
        }

        [TestMethod]
        public void AmqpWebSocketTransportTest()
        {
            string queue = "AmqpWebSocketTransportTest";
            broker.AddQueue(queue);

            AmqpConnection connection = AmqpConnection.Factory.OpenConnectionAsync(
                TestAmqpBrokerFixture.WsAddress.OriginalString).GetAwaiter().GetResult();

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            session.Open();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            sLink.Open();

            int messageCount = 1800;
            for (int i = 0; i < messageCount; i++)
            {
                AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "message" + i });
                sLink.SendMessageAsync(message).Wait();
            }

            sLink.Close();

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            rLink.Open();

            for (int i = 0; i < messageCount; i++)
            {
                AmqpMessage message2 = rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(60)).GetAwaiter().GetResult();
                Assert.IsNotNull(message2);

                rLink.AcceptMessage(message2);
                message2.Dispose();
            }

            rLink.Close();

            connection.Close();
        }
    }
}
