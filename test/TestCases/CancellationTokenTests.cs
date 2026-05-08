// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Sasl;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;
    using TestAmqpBroker;

    [TestClass]
    public class CancellationTokenTests
    {
        static CancellationTokenTests()
        {
            // AmqpTrace.FrameLogger = s => System.Diagnostics.Trace.WriteLine(s);
        }

        Uri addressUri = new Uri("amqp://guest:guest@localhost:5678");

        [TestMethod]
        public Task TransportTest()
        {
            return this.RunTransportTest(false);
        }

        [TestMethod]
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
                await Assert.ThrowsExceptionAsync<TaskCanceledException>(async () =>
                {
                    AmqpSettings settings = new AmqpSettings();
                    settings.TransportProviders.Add(new SaslTransportProvider(AmqpVersion.V100));
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

        [TestMethod]
        public Task ConnectionFactoryTest()
        {
            return this.RunConnectionFactoryTest(false);
        }

        [TestMethod]
        public Task ConnectionFactoryCanceledTest()
        {
            return this.RunConnectionFactoryTest(true);
        }

        async Task RunConnectionFactoryTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener(saslDelayMs: 200);

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task ConnectionOpenTest()
        {
            return this.RunConnectionOpenTest(false);
        }

        [TestMethod]
        public Task ConnectionOpenCanceledTest()
        {
            return this.RunConnectionOpenTest(true);
        }

        async Task RunConnectionOpenTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener(saslDelayMs: 200);

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task ConnectionCloseTest()
        {
            return this.RunConnectionCloseTest(false);
        }

        [TestMethod]
        public Task ConnectionCloseCanceledTest()
        {
            return this.RunConnectionCloseTest(true);
        }

        async Task RunConnectionCloseTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener();

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task SessionOpenTest()
        {
            return this.RunSessionOpenTest(false);
        }

        [TestMethod]
        public Task SessionOpenCanceledTest()
        {
            return this.RunSessionOpenTest(true);
        }

        async Task RunSessionOpenTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener();

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task SessionCloseTest()
        {
            return this.RunSessionCloseTest(false);
        }

        [TestMethod]
        public Task SessionCloseCanceledTest()
        {
            return this.RunSessionCloseTest(true);
        }

        async Task RunSessionCloseTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener();

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task LinkOpenTest()
        {
            return this.RunLinkOpenTest(false);
        }

        [TestMethod]
        public Task LinkOpenCanceledTest()
        {
            return this.RunLinkOpenTest(true);
        }

        async Task RunLinkOpenTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener();

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task LinkCloseTest()
        {
            return this.RunLinkCloseTest(false);
        }

        [TestMethod]
        public Task LinkCloseCanceledTest()
        {
            return this.RunLinkCloseTest(true);
        }

        async Task RunLinkCloseTest(bool cancelBefore)
        {
            AmqpConnectionListener listener = OpenListener();

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task LinkSendTest()
        {
            return this.RunLinkSendTest(false);
        }

        [TestMethod]
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

            AmqpConnectionListener listener = OpenListener(provider);

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        [TestMethod]
        public Task LinkReceiveTest()
        {
            return this.RunLinkReceiveTest(false);
        }

        [TestMethod]
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

            AmqpConnectionListener listener = OpenListener(provider);

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

                await Assert.ThrowsExceptionAsync<TaskCanceledException>(async () => await task);
            }
            finally
            {
                listener.Close();
            }
        }

        [TestMethod]
        public Task LinkDispositionTest()
        {
            return this.RunLinkDispositionTest(false);
        }

        [TestMethod]
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

            AmqpConnectionListener listener = OpenListener(provider);

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri);
                    var session = connection.CreateSession(new AmqpSessionSettings());
                    await session.OpenAsync();

                    var link = new ReceivingAmqpLink(session, new AmqpLinkSettings() { Role = true, LinkName = "receiver", TotalLinkCredit = 0, Source = new Source(), Target = new Target() });
                    await link.OpenAsync();

                    var message = await link.ReceiveMessageAsync();
                    Assert.IsNotNull(message);

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

        [TestMethod]
        public Task LinkDrainTest()
        {
            return this.RunLinkDrainTest(false);
        }

        [TestMethod]
        public Task LinkDrainCanceledTest()
        {
            return this.RunLinkDrainTest(true);
        }

        [TestMethod]
        public Task LinkDrainTimeoutTest()
        {
            return this.RunLinkDrainTest(false, 800);
        }

        async Task RunLinkDrainTest(bool cancelBefore, int timeoutMilliseconds = 0)
        {
            var provider = new TestRuntimeProvider()
            {
                LinkFactory = (s, t) => { t.SettleType = SettleMode.SettleOnDispose; return new TestLink(s, t, flowHang: true); }
            };

            AmqpConnectionListener listener = OpenListener(provider);

            try
            {
                try
                {
                    var factory = new AmqpConnectionFactory();
                    var connection = await factory.OpenConnectionAsync(this.addressUri, AmqpConstants.DefaultTimeout);
                    var session = connection.CreateSession(new AmqpSessionSettings());
                    await session.OpenAsync(AmqpConstants.DefaultTimeout);

                    var link = new ReceivingAmqpLink(session, new AmqpLinkSettings() { Role = true, LinkName = "receiver", TotalLinkCredit = 0, Source = new Source(), Target = new Target() });
                    await link.OpenAsync(AmqpConstants.DefaultTimeout);

                    var cts = new CancellationTokenSource();
                    if (cancelBefore && timeoutMilliseconds == 0)
                    {
                        cts.Cancel();
                    }

                    if (timeoutMilliseconds > 0)
                    {
                        link.Settings.OperationTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
                    }

                    var task = link.DrainAsync(cts.Token);
                    if (!cancelBefore && timeoutMilliseconds == 0)
                    {
                        await Task.Yield();
                        cts.Cancel();
                    }

                    await task;

                    Assert.IsTrue(false, "Exception not thrown");
                }
                catch (Exception exception)
                {
                    if (timeoutMilliseconds > 0)
                    {
                        Assert.AreEqual(typeof(TimeoutException), exception.GetType());
                    }
                    else
                    {
                        Assert.AreEqual(typeof(TaskCanceledException), exception.GetType());
                    }
                }
            }
            finally
            {
                listener.Close();
            }
        }

        [TestMethod]
        public async Task CbsSendTokenNoCancelTest()
        {
            var broker = new TestAmqpBroker(new[] { addressUri.AbsoluteUri }, addressUri.UserInfo, null, null);
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

        [TestMethod]
        public Task CbsSendTokenTest()
        {
            return this.RunCbsSendTokenTest(false);
        }

        [TestMethod]
        public Task CbsSendTokenCancelledTest()
        {
            return this.RunCbsSendTokenTest(true);
        }

        async Task RunCbsSendTokenTest(bool cancelBefore)
        {
            var broker = new TestAmqpBroker(new[] { addressUri.AbsoluteUri }, addressUri.UserInfo, null, null);
            broker.AddNode(new CbsNode() { ProcessingTime = TimeSpan.FromSeconds(10) });
            broker.Start();

            try
            {
                await AssertExtensions.ThrowsAnyAsync<TaskCanceledException>(async () =>
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

        AmqpConnectionListener OpenListener(TestRuntimeProvider runtimeProvider = null, int saslDelayMs = 0)
        {
            AmqpSettings settings = new AmqpSettings { RuntimeProvider = runtimeProvider ?? new TestRuntimeProvider() };
            var saslProvider = new SaslTransportProvider(AmqpVersion.V100);
            saslProvider.AddHandler(new SaslPlainHandler(new TestSaslPlainAuthenticator() { DelayInMilliseconds = saslDelayMs }));
            settings.TransportProviders.Add(saslProvider);
            var listener = new AmqpConnectionListener(new[] { addressUri.AbsoluteUri }, settings, new AmqpConnectionSettings());
            listener.Open();
            return listener;
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
            readonly bool flowHang;

            public TestLink(AmqpSession session, AmqpLinkSettings settings, bool openHang = false, bool closeHang = false,
                bool sendHang = false, bool receiveHang = false, bool disposeHang = false, bool flowHang = false)
                : base(session, settings)
            {
                this.openHang = openHang;
                this.closeHang = closeHang;
                this.sendHang = sendHang;
                this.receiveHang = receiveHang;
                this.disposeHang = disposeHang;
                this.flowHang = flowHang;
            }

            protected override bool OpenInternal()
            {
                return this.openHang ? false : base.OpenInternal();
            }

            protected override bool CloseInternal()
            {
                return this.closeHang ? false : base.CloseInternal();
            }

            public override bool CreateDelivery(Transfer transfer, out Delivery delivery)
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

            protected override void OnReceiveFlow(Flow flow)
            {
                if (!this.flowHang)
                {
                    base.OnReceiveFlow(flow);
                }
            }

            internal override void ProcessUnsettledDeliveries(Attach remoteAttach)
            {
                throw new NotImplementedException();
            }
        }
    }
}
