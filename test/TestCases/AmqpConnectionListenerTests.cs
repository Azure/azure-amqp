// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.Azure.Amqp.Sasl;
    using Xunit;
    using System.Security.Principal;

    [Trait("Category", TestCategory.Current)]
    public class AmqpConnectionListenerTests
    {
        static readonly ArraySegment<byte> NullBinary = new ArraySegment<byte>();
        static readonly ArraySegment<byte> EmptyBinary = new ArraySegment<byte>(new byte[0]);

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
            Outcome outcome = await sLink.SendMessageAsync(message, EmptyBinary, NullBinary, TimeSpan.FromSeconds(10));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnDispose, 10));
            await rLink.OpenAsync(TimeSpan.FromSeconds(20));

            var receivedMessage = await rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(20));
            Assert.NotNull(receivedMessage);
            outcome = await rLink.DisposeMessageAsync(receivedMessage.DeliveryTag, new Accepted(), false, TimeSpan.FromSeconds(20));
            Assert.Equal(Accepted.Code, outcome.DescriptorCode);

            await connection.CloseAsync(TimeSpan.FromSeconds(20));
        }

        class TestRuntimeProvider : IRuntimeProvider
        {
            Queue<AmqpMessage> messages = new Queue<AmqpMessage>();

            public AmqpConnection CreateConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings)
            {
                connectionSettings.ContainerId = this.GetType().Name;
                return new AmqpConnection(transport, protocolHeader, false, amqpSettings, connectionSettings);
            }

            public AmqpSession CreateSession(AmqpConnection connection, AmqpSessionSettings settings)
            {
                return new AmqpSession(connection, settings, this);
            }

            public AmqpLink CreateLink(AmqpSession session, AmqpLinkSettings settings)
            {
                AmqpLink link;
                if (settings.Role.Value)
                {
                    var receiver = new ReceivingAmqpLink(session, settings);
                    receiver.RegisterMessageListener(m =>
                    {
                        this.messages.Enqueue(m.Clone());
                        receiver.AcceptMessage(m, true, true);
                        m.Dispose();
                    });
                    link = receiver;
                }
                else
                {
                    var sender = new SendingAmqpLink(session, settings);
                    sender.RegisterCreditListener((credit, drain, tx) =>
                    {
                        AmqpMessage message = this.messages.Dequeue();
                        message.DeliveryAnnotations.Map["x-opt-sequence-number"] = 1;
                        sender.SendMessageNoWait(message, EmptyBinary, NullBinary);
                    });
                    sender.RegisterDispositionListener(d =>
                    {
                        sender.DisposeDelivery(d, true, d.State);
                    });
                    link = sender;
                }

                return link;
            }

            public IAsyncResult BeginOpenLink(AmqpLink link, TimeSpan timeout, AsyncCallback callback, object state)
            {
                var result = new CompletedAsyncResult(callback, state);
                callback(result);
                return result;
            }

            public void EndOpenLink(IAsyncResult result)
            {
            }
        }

        class TestSaslPlainAuthenticator : ISaslPlainAuthenticator
        {
            public Task<IPrincipal> AuthenticateAsync(string identity, string password)
            {
                if (identity != password)
                {
                    throw new UnauthorizedAccessException();
                }

                IPrincipal principal = new GenericPrincipal(new GenericIdentity(identity), new string[] { "SEND", "RECV" });
                return Task.FromResult(principal);
            }
        }

        class CompletedAsyncResult : IAsyncResult
        {
            public CompletedAsyncResult(AsyncCallback callback, object state)
            {
                this.AsyncState = state;
            }

            public bool IsCompleted => true;

            public WaitHandle AsyncWaitHandle => throw new NotImplementedException();

            public object AsyncState { get; set; }

            public bool CompletedSynchronously => true;
        }
    }
}
