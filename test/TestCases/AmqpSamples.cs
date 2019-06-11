// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using TestAmqpBroker;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpSamples : IClassFixture<TestAmqpBrokerFixture>
    {
        Uri addressUri;
        TestAmqpBroker broker;

        public AmqpSamples(TestAmqpBrokerFixture testAmqpBrokerFixture)
        {
            addressUri = TestAmqpBrokerFixture.Address;
            broker = testAmqpBrokerFixture.Broker;
        }

        [Fact]
        public async Task SendReceiveSample()
        {
            string queue = "SendReceiveSample";
            broker.AddQueue(queue);

            var factory = new AmqpConnectionFactory();
            var connection = await factory.OpenConnectionAsync(addressUri);
            var session = await connection.OpenSessionAsync();

            var sender = await session.OpenLinkAsync<SendingAmqpLink>("sender", queue);
            var outcome = await sender.SendMessageAsync(AmqpMessage.Create("Hello World!"));
            await sender.CloseAsync();

            var receiver = await session.OpenLinkAsync<ReceivingAmqpLink>("receiver", queue);
            var message = await receiver.ReceiveMessageAsync();
            string body = (string)message.ValueBody.Value;
            receiver.AcceptMessage(message);
            await receiver.CloseAsync();

            await connection.CloseAsync();
        }
    }
}
