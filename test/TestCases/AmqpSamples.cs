// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using TestAmqpBroker;
    using Xunit;

    [Collection("Sequential")]
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
            string queue = "LinkRecoverySample";
            broker.AddQueue(queue);
            var factory = new AmqpConnectionFactory();
            // Create the AMQP connection with EnableLinkRecovery = true in its settings to allow link recovery.
            var connection = await factory.OpenConnectionAsync(addressUri, new AmqpConnectionSettings() { EnableLinkRecovery = true }, TimeSpan.FromMinutes(1));

            try
            {
                // Send and receive the message normally.
                var session = await connection.OpenSessionAsync();
                var sender = await session.OpenLinkAsync<SendingAmqpLink>("sender", queue);
                await sender.SendMessageAsync(AmqpMessage.Create("Hello World!"));
                var receiver = await session.OpenLinkAsync<ReceivingAmqpLink>("receiver", queue);
                var message = await receiver.ReceiveMessageAsync();

                // Restart the broker. All connections should be disconnected from the broker side.
                broker.Stop();
                await Task.Delay(1000);
                broker.Start();

                // Try to complete the received message now. Should throw exception because the link is closed.
                Assert.Throws<AmqpException>(() => receiver.AcceptMessage(message));

                // We need to reconnect with the same connection and link settings for link recovery.
                AmqpConnectionSettings connectionRecoverySettings = connection.CreateSettingsForRecovery();
                connection = await factory.OpenConnectionAsync(addressUri, connectionRecoverySettings, AmqpConstants.DefaultTimeout);
                session = await connection.OpenSessionAsync();
                receiver = await session.RecoverLinkAsync<ReceivingAmqpLink>(receiver);
                receiver.AcceptMessage(message);
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                throw;
            }
            finally
            {
                await connection.CloseAsync();
            }
        }
    }
}
