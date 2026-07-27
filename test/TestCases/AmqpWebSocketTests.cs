// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
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
        public async Task AmqpWebSocketTransportTest()
        {
            string queue = "AmqpWebSocketTransportTest";
            broker.AddQueue(queue);

            AmqpConnection connection = await AmqpConnection.Factory.OpenConnectionAsync(
                TestAmqpBrokerFixture.WsAddress.OriginalString);

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            await session.OpenAsync();

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            await sLink.OpenAsync();

            int messageCount = 1800;
            for (int i = 0; i < messageCount; i++)
            {
                AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "message" + i });
                await sLink.SendMessageAsync(message);
            }

            await sLink.CloseAsync();

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
            await rLink.OpenAsync();

            for (int i = 0; i < messageCount; i++)
            {
                AmqpMessage message2 = await rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(60));
                Assert.IsNotNull(message2);

                rLink.AcceptMessage(message2);
                message2.Dispose();
            }

            await rLink.CloseAsync();

            await connection.CloseAsync();
        }
    }
}
