// Copyright (c) Microsoft. All rights reserved.
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

    [Trait("Category", TestCategory.Current)]
    public class CancellationTokenTests
    {
        static CancellationTokenTests()
        {
            // AmqpTrace.FrameLogger = s => System.Diagnostics.Trace.WriteLine(s);
        }

        Uri addressUri = new Uri("amqp://localhost:5678");

        [Fact]
        public async Task TransportTest()
        {
            var transportSettings = new TcpTransportSettings() { Host = addressUri.Host, Port = addressUri.Port };
            TcpTransportListener listener = new TcpTransportListener(transportSettings);
            listener.Open();
            listener.Listen((n, a) => { });

            try
            {
                await Assert.ThrowsAsync<TaskCanceledException>(() =>
                {
                    AmqpSettings settings = new AmqpSettings();
                    settings.TransportProviders.Add(new AmqpTransportProvider(AmqpVersion.V100));

                    AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
                    var cts = new CancellationTokenSource();
                    Task task = initiator.ConnectAsync(cts.Token);
                    cts.Cancel();
                    return task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task ConnectionFactoryTest()
        {
            AmqpConnectionListener listener = new AmqpConnectionListener(addressUri.AbsoluteUri, new TestRuntimeProvider());
            listener.Open();

            try
            {
                await Assert.ThrowsAnyAsync<TaskCanceledException>(() =>
                {
                    var factory = new AmqpConnectionFactory();
                    var cts = new CancellationTokenSource();
                    Task task = factory.OpenConnectionAsync(this.addressUri, cts.Token);
                    cts.Cancel();
                    return task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task ConnectionOpenTest()
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
                    var task = connection.OpenAsync(cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task ConnectionCloseTest()
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
                    var task = connection.CloseAsync(cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task SessionOpenTest()
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
                    var task = session.OpenAsync(cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task SessionCloseTest()
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
                    var task = session.CloseAsync(cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task LinkOpenTest()
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
                    var task = link.OpenAsync(cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task LinkCloseTest()
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
                    var task = link.CloseAsync(cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task LinkSendTest()
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
                    var task = link.SendMessageAsync(AmqpMessage.Create(new AmqpValue() { Value = "test" }), AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task LinkReceiveTest()
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
                var task = link.ReceiveMessageAsync(cts.Token);
                await Task.Delay(100);
                cts.Cancel();
                var completedTask = await Task.WhenAny(task, Task.Delay(5000));
                Assert.Equal(task, completedTask);
                Assert.Null(task.Result);
            }
            finally
            {
                listener.Close();
            }
        }

        [Fact]
        public async Task LinkDispositionTest()
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
                    var task = link.DisposeMessageAsync(message.DeliveryTag, AmqpConstants.AcceptedOutcome, cts.Token);
                    await Task.Delay(100);
                    cts.Cancel();
                    await task;
                });
            }
            finally
            {
                listener.Close();
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
        }
    }
}
