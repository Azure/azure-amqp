﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Transport;
    using Xunit;
    using TestAmqpBroker;

    [Trait("Category", TestCategory.Current)]
    public class CancellationTokenTests
    {
        static CancellationTokenTests()
        {
            // AmqpTrace.FrameLogger = s => System.Diagnostics.Trace.WriteLine(s);
        }

        Uri addressUri = new Uri("amqp://localhost:5678");

        [Fact]
        public Task TransportTest()
        {
            return this.RunTransportTest(false);
        }

        [Fact]
        public Task TransportCanceledTest()
        {
            return this.RunTransportTest(true);
        }

        async Task RunTransportTest(bool cancelBefore)
        {
            var transportSettings = new TcpTransportSettings() { Host = addressUri.Host, Port = addressUri.Port };
            TcpTransportListener listener = new TcpTransportListener(transportSettings);
            listener.Open();
            listener.Listen((n, a) => { });

            try
            {
                await Assert.ThrowsAsync<TaskCanceledException>(async () =>
                {
                    AmqpSettings settings = new AmqpSettings();
                    settings.TransportProviders.Add(new AmqpTransportProvider(AmqpVersion.V100));

                    AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    Task task = initiator.ConnectAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task ConnectionFactoryTest()
        {
            return this.RunConnectionFactoryTest(false);
        }

        [Fact]
        public Task ConnectionFactoryCanceledTest()
        {
            return this.RunConnectionFactoryTest(true);
        }

        async Task RunConnectionFactoryTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = factory.OpenConnectionAsync(this.addressUri, cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task ConnectionOpenTest()
        {
            return this.RunConnectionOpenTest(false);
        }

        [Fact]
        public Task ConnectionOpenCanceledTest()
        {
            return this.RunConnectionOpenTest(true);
        }

        async Task RunConnectionOpenTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var transportSettings = new TcpTransportSettings() { Host = addressUri.Host, Port = addressUri.Port };
                    AmqpSettings settings = new AmqpSettings();
                    settings.TransportProviders.Add(new AmqpTransportProvider(AmqpVersion.V100));

                    AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
                    TransportBase transport = await initiator.ConnectAsync(CancellationToken.None);
                    var connection = new TestConnection(transport, settings, new AmqpConnectionSettings() { ContainerId = "test", HostName = addressUri.Host }, openHang: true);

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = connection.OpenAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task ConnectionCloseTest()
        {
            return this.RunConnectionCloseTest(false);
        }

        [Fact]
        public Task ConnectionCloseCanceledTest()
        {
            return this.RunConnectionCloseTest(true);
        }

        async Task RunConnectionCloseTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var transportSettings = new TcpTransportSettings() { Host = addressUri.Host, Port = addressUri.Port };
                    AmqpSettings settings = new AmqpSettings();
                    settings.TransportProviders.Add(new AmqpTransportProvider(AmqpVersion.V100));

                    AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
                    TransportBase transport = await initiator.ConnectAsync(CancellationToken.None);
                    var connection = new TestConnection(transport, settings, new AmqpConnectionSettings() { ContainerId = "test", HostName = addressUri.Host }, closeHang: true);
                    await connection.OpenAsync();

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = connection.CloseAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task SessionOpenTest()
        {
            return this.RunSessionOpenTest(false);
        }

        [Fact]
        public Task SessionOpenCanceledTest()
        {
            return this.RunSessionOpenTest(true);
        }

        async Task RunSessionOpenTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);

                    var session = new TestSession(connection, new AmqpSessionSettings(), openHang: true);
                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = session.OpenAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task SessionCloseTest()
        {
            return this.RunSessionCloseTest(false);
        }

        [Fact]
        public Task SessionCloseCanceledTest()
        {
            return this.RunSessionCloseTest(true);
        }

        async Task RunSessionCloseTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);
                    var session = new TestSession(connection, new AmqpSessionSettings(), closeHang: true);
                    await session.OpenAsync();

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = session.CloseAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task LinkOpenTest()
        {
            return this.RunLinkOpenTest(false);
        }

        [Fact]
        public Task LinkOpenCanceledTest()
        {
            return this.RunLinkOpenTest(true);
        }

        async Task RunLinkOpenTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);
                    var session = connection.CreateSession(new AmqpSessionSettings());
                    await session.OpenAsync();

                    var link = new TestLink(session, new AmqpLinkSettings() { Role = false, LinkName = "sender", Source = new Source(), Target = new Target() }, openHang: true);
                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = link.OpenAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task LinkCloseTest()
        {
            return this.RunLinkCloseTest(false);
        }

        [Fact]
        public Task LinkCloseCanceledTest()
        {
            return this.RunLinkCloseTest(true);
        }

        async Task RunLinkCloseTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);
                    var session = connection.CreateSession(new AmqpSessionSettings());
                    await session.OpenAsync();

                    var link = new TestLink(session, new AmqpLinkSettings() { Role = false, LinkName = "sender", Source = new Source(), Target = new Target() }, closeHang: true);
                    await link.OpenAsync();

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = link.CloseAsync(cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task LinkSendTest()
        {
            return this.RunLinkSendTest(false);
        }

        [Fact]
        public Task LinkSendCanceledTest()
        {
            return this.RunLinkSendTest(true);
        }

        async Task RunLinkSendTest(bool cancelBefore)
        {
            var provider = new TestRuntimeProvider()
            {
                LinkFactory = (s, t) => { t.TotalLinkCredit = 10; return new TestLink(s, t, sendHang: true); }
            };
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, provider);
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);
                    var session = connection.CreateSession(new AmqpSessionSettings());
                    await session.OpenAsync();

                    var link = new SendingAmqpLink(session, new AmqpLinkSettings() { Role = false, LinkName = "sender", Source = new Source(), Target = new Target() });
                    await link.OpenAsync();

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = link.SendMessageAsync(AmqpMessage.Create(new AmqpValue() { Value = "test" }), AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task LinkReceiveTest()
        {
            return this.RunLinkReceiveTest(false);
        }

        [Fact]
        public Task LinkReceiveCanceledTest()
        {
            return this.RunLinkReceiveTest(true);
        }

        async Task RunLinkReceiveTest(bool cancelBefore)
        {
            var provider = new TestRuntimeProvider()
            {
                LinkFactory = (s, t) => new TestLink(s, t, receiveHang: true)
            };
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, provider);
            listener.Open();

            try
            {
                var factory = new AmqpConnectionFactory();
                var connection = await factory.OpenConnectionAsync(this.addressUri);
                var session = connection.CreateSession(new AmqpSessionSettings());
                await session.OpenAsync();

                var link = new ReceivingAmqpLink(session, new AmqpLinkSettings() { Role = true, LinkName = "receiver", TotalLinkCredit = 10, Source = new Source(), Target = new Target() });
                await link.OpenAsync();

                var cts = new CancellationTokenSource();
                if (cancelBefore)
                {
                    cts.Cancel();
                }

                var task = link.ReceiveMessageAsync(cts.Token);
                if (!cancelBefore)
                {
                    await Task.Yield();
                    cts.Cancel();
                }

                await Assert.ThrowsAsync<TaskCanceledException>(async () => await task);
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public Task LinkDispositionTest()
        {
            return this.RunLinkDispositionTest(false);
        }

        [Fact]
        public Task LinkDispositionCanceledTest()
        {
            return this.RunLinkDispositionTest(true);
        }

        async Task RunLinkDispositionTest(bool cancelBefore)
        {
            var provider = new TestRuntimeProvider()
            {
                LinkFactory = (s, t) => { t.SettleType = SettleMode.SettleOnDispose; return new TestLink(s, t, disposeHang: true); }
            };
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, provider);
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);
                    var session = connection.CreateSession(new AmqpSessionSettings());
                    await session.OpenAsync();

                    var link = new ReceivingAmqpLink(session, new AmqpLinkSettings() { Role = true, LinkName = "receiver", TotalLinkCredit = 0, Source = new Source(), Target = new Target() });
                    await link.OpenAsync();

                    var message = await link.ReceiveMessageAsync();
                    Assert.NotNull(message);

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = link.DisposeMessageAsync(message.DeliveryTag, AmqpConstants.AcceptedOutcome, cts.Token);
                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task CbsSendTokenNoCancelTest()
        {
            var broker = new TestAmqpBroker(new[] { addressUri.AbsoluteUri }, null, null, null);
            broker.AddNode(new CbsNode());
            broker.Start();

            try
            {
                var factory = new AmqpConnectionFactory();
                var connection = await factory.OpenConnectionAsync(addressUri, CancellationToken.None);
                var cbsLink = new AmqpCbsLink(connection);
                await cbsLink.SendTokenAsync(new TestTokenProvider(), addressUri, addressUri.OriginalString,
                    addressUri.OriginalString, new[] { "Send" }, CancellationToken.None);
                await connection.CloseAsync(CancellationToken.None);
            }
            finally
            {
                broker.Stop();
            }
        }

        [Fact]
        public Task CbsSendTokenTest()
        {
            return this.RunCbsSendTokenTest(false);
        }

        [Fact]
        public Task CbsSendTokenCancelledTest()
        {
            return this.RunCbsSendTokenTest(true);
        }

        async Task RunCbsSendTokenTest(bool cancelBefore)
        {
            var broker = new TestAmqpBroker(new[] { addressUri.AbsoluteUri }, null, null, null);
            broker.AddNode(new CbsNode() { ProcessingTime = TimeSpan.FromSeconds(10) });
            broker.Start();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri, CancellationToken.None);
                    var cbsLink = new AmqpCbsLink(connection);

                    var cts = new CancellationTokenSource();
                    if (cancelBefore)
                    {
                        cts.Cancel();
                    }

                    var task = cbsLink.SendTokenAsync(new TestTokenProvider(), addressUri, addressUri.OriginalString,
                        addressUri.OriginalString, new[] { "Send" }, cts.Token);

                    if (!cancelBefore)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;
                });
            }
            finally
            {
                broker.Stop();
            }
        }

        class TestTokenProvider : ICbsTokenProvider
        {
            public Task<CbsToken> GetTokenAsync(Uri namespaceAddress, string appliesTo, string[] requiredClaims)
            {
                var token = new CbsToken("test:token", "tt", DateTime.UtcNow.AddHours(1));
                return Task.FromResult(token);
            }
        }

        class TestConnection : AmqpConnection
        {
            readonly bool openHang;
            readonly bool closeHang;

            public TestConnection(TransportBase transport, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings, bool openHang = false, bool closeHang = false)
                : base(transport, amqpSettings, connectionSettings)
            {
                this.openHang = openHang;
                this.closeHang = closeHang;
            }

            protected override bool OpenInternal()
            {
                return this.openHang ? false : base.OpenInternal();
            }

            protected override bool CloseInternal()
            {
                return this.closeHang ? false : base.CloseInternal();
            }
        }

        class TestSession : AmqpSession
        {
            readonly bool openHang;
            readonly bool closeHang;

            public TestSession(AmqpConnection connection, AmqpSessionSettings settings, bool openHang = false, bool closeHang = false)
                : base(connection, settings, null)
            {
                this.openHang = openHang;
                this.closeHang = closeHang;
                connection.AddSession(this, null);
            }

            protected override bool OpenInternal()
            {
                return this.openHang ? false : base.OpenInternal();
            }

            protected override bool CloseInternal()
            {
                return this.closeHang ? false : base.CloseInternal();
            }
        }

        class TestLink : AmqpLink
        {
            readonly bool openHang;
            readonly bool closeHang;
            readonly bool sendHang;
            readonly bool receiveHang;
            readonly bool disposeHang;

            public TestLink(AmqpSession session, AmqpLinkSettings settings, bool openHang = false, bool closeHang = false,
                bool sendHang = false, bool receiveHang = false, bool disposeHang = false)
                : base(session, settings)
            {
                this.openHang = openHang;
                this.closeHang = closeHang;
                this.sendHang = sendHang;
                this.receiveHang = receiveHang;
                this.disposeHang = disposeHang;
            }

            protected override bool OpenInternal()
            {
                return this.openHang ? false : base.OpenInternal();
            }

            protected override bool CloseInternal()
            {
                return this.closeHang ? false : base.CloseInternal();
            }

            protected override bool CreateDelivery(Transfer transfer, out Delivery delivery)
            {
                delivery = AmqpMessage.CreateReceivedMessage();
                return true;
            }

            protected override void OnProcessTransfer(Delivery delivery, Transfer transfer, Frame rawFrame)
            {
                if (!this.sendHang)
                {
                    this.DisposeDelivery(delivery, true, AmqpConstants.AcceptedOutcome);
                }
            }

            protected override void OnCreditAvailable(int session, uint link, bool drain, ArraySegment<byte> txnId)
            {
                for (uint i = 0; !this.receiveHang && i < link; i++)
                {
                    var message = AmqpMessage.Create(new AmqpValue() { Value = "test" });
                    message.DeliveryTag = new ArraySegment<byte>(Guid.NewGuid().ToByteArray());
                    message.Settled = false;
                    this.TrySendDelivery(message);
                }
            }

            protected override void OnDisposeDeliveryInternal(Delivery delivery)
            {
                if (!this.disposeHang)
                {
                    this.DisposeDelivery(delivery, true, AmqpConstants.AcceptedOutcome);
                }
            }

            protected override void ProcessUnsettledDeliveries(Attach remoteAttach)
            {
                throw new NotImplementedException();
            }
        }
    }
}
