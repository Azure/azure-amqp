// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Sasl;
    using global::Microsoft.Azure.Amqp.Transport;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpConnectionListenerTests
    {
        string queue = "q1";

        [Fact]
        public async Task ListenerTcpTest()
        {
            string address = "amqp://localhost:5676";

            AmqpConnectionListener listener = new AmqpConnectionListener(address, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await RunClientAsync(address);
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task ListenerTlsTest()
        {
            string address = "amqps://localhost:5676";

            AmqpSettings settings = AmqpUtils.GetAmqpSettings(false, "localhost", false);
            AmqpConnectionSettings connectionSettings = AmqpUtils.GetConnectionSettings(32 * 1024);
            settings.RuntimeProvider = new TestRuntimeProvider();
            AmqpConnectionListener listener = new AmqpConnectionListener(new[] { address }, settings, connectionSettings);
            listener.Open();

            try
            {
                await RunClientAsync(address);
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task ListenerSaslTlsTest()
        {
            string address = "amqps://guest:guest@localhost:5676";

            AmqpSettings settings = AmqpUtils.GetAmqpSettings(false, "localhost", false, new SaslPlainHandler(new TestSaslPlainAuthenticator()));
            AmqpConnectionSettings connectionSettings = AmqpUtils.GetConnectionSettings(32 * 1024);
            settings.RuntimeProvider = new TestRuntimeProvider();
            AmqpConnectionListener listener = new AmqpConnectionListener(new[] { address }, settings, connectionSettings);
            listener.Open();

            try
            {
                await RunClientAsync(address);
            }
            finally
            {
                listener.Close();
            }
        }

        async Task RunClientAsync(string address)
        {
            AmqpConnectionFactory factory = new AmqpConnectionFactory();
            factory.Settings.TransportProviders.Add(
                new TlsTransportProvider(
                    new TlsTransportSettings() { CertificateValidationCallback = (a, b, c, d) => true },
                    AmqpVersion.V100));

            AmqpConnection connection = await factory.OpenConnectionAsync(address);

            AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
            await session.OpenAsync(TimeSpan.FromSeconds(20));

            SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
            await sLink.OpenAsync(TimeSpan.FromSeconds(20));

            AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "AmqpConnectionFactoryTest" });
            Outcome outcome = await sLink.SendMessageAsync(message, TestRuntimeProvider.EmptyBinary, TestRuntimeProvider.NullBinary, TimeSpan.FromSeconds(10));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnDispose, 10));
            await rLink.OpenAsync(TimeSpan.FromSeconds(20));

            var receivedMessage = await rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(20));
            Assert.NotNull(receivedMessage);
            outcome = await rLink.DisposeMessageAsync(receivedMessage.DeliveryTag, new Accepted(), false, TimeSpan.FromSeconds(20));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            await connection.CloseAsync(TimeSpan.FromSeconds(20));
        }
    }
}
