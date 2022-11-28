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

        [Fact]
        public async Task LinkRecoverySample()
        {
            string queueName = "LinkRecoverySample";
            broker.AddQueue(queueName);
            broker.TerminusStore = new AmqpInMemoryTerminusStore();

            // Need to provide a link terminus manager and unsettled delivery store in order to track the link terminus and unsettled deliveries in order to do link recovery.
            // The link terminus manager and unsettled delivery store should be managed by the client/application that is using this AMQP library.
            var amqpSettings = new AmqpSettings() { RuntimeProvider = new TestLinkRecoveryRuntimeProvider(new AmqpInMemoryTerminusStore()) };
            var factory = new AmqpConnectionFactory(amqpSettings);

            // Need to use the same containId later to identify and recover this link endpoint.
            string containerId = Guid.NewGuid().ToString();
            var connection = await factory.OpenConnectionAsync(addressUri, new AmqpConnectionSettings() { ContainerId = containerId }, TimeSpan.FromMinutes(1));

            try
            {
                AmqpSession session = await connection.OpenSessionAsync();

                // Specify the desired link expiry policy (required for link recovery) and link expiry timeout (optional for link recovery) on the link settings for potential recovery of this link in the future.
                AmqpLinkSettings linkSettings = AmqpLinkSettings.Create<ReceivingAmqpLink>("receiver", queueName);
                linkSettings.SetExpiryPolicy(LinkTerminusExpiryPolicy.Never);
                var receiver = await session.OpenLinkAsync<ReceivingAmqpLink>(linkSettings);

                // Send and receive the message as normal.
                var sender = await session.OpenLinkAsync<SendingAmqpLink>("receiver", queueName);
                await sender.SendMessageAsync(AmqpMessage.Create("Hello World!"));
                var message = await receiver.ReceiveMessageAsync();

                // Restart the broker. All connections should be disconnected from the broker side.
                broker.Stop();
                await Task.Delay(1000);
                broker.Start();

                // Try to complete the received message now. Should throw exception because the link is closed.
                Assert.Throws<AmqpException>(() => receiver.AcceptMessage(message));

                // Need to reconnect with the same containerId and link identifier for link recovery.
                AmqpConnectionSettings connectionRecoverySettings = new AmqpConnectionSettings() { ContainerId = containerId };
                connection = await factory.OpenConnectionAsync(addressUri, connectionRecoverySettings, AmqpConstants.DefaultTimeout);
                AmqpSession newSession = await connection.OpenSessionAsync();
                var recoveredReceiver = await newSession.OpenLinkAsync<ReceivingAmqpLink>(receiver.Settings);
                recoveredReceiver.AcceptMessage(message);
            }
            finally
            {
                await connection.CloseAsync();
            }
        }
    }
}
