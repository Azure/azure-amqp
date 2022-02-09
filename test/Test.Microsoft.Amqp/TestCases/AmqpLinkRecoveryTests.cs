// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Amqp.TestCases
{
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Transaction;
    using global::Microsoft.Azure.Amqp.Transport;
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Test.Microsoft.Azure.Amqp;
    using TestAmqpBroker;
    using Xunit;
    using static TestAmqpBroker.TestAmqpBroker;

    [Collection("Sequential")]
    [Trait("Category", TestCategory.Current)]
    public class AmqpLinkRecoveryTests : IClassFixture<TestAmqpBrokerFixture>
    {
        static Uri addressUri;
        static TestAmqpBroker broker;

        public AmqpLinkRecoveryTests(TestAmqpBrokerFixture testAmqpBrokerFixture)
        {
            addressUri = TestAmqpBrokerFixture.Address;
            broker = testAmqpBrokerFixture.Broker;
        }

        // Test that the connection settings obtained from AmqpConnection.CreateSettingsForRecovery() can be used to open new connections with identical settings.
        [Fact]
        public async Task ConnectionRecoveryTest()
        {
            AmqpConnectionSettings connectionSettings = new AmqpConnectionSettings()
            {
                ContainerId = nameof(ConnectionRecoveryTest),
            };

            AmqpConnection originalConnection = await AmqpConnection.Factory.OpenConnectionAsync(addressUri, connectionSettings, TimeSpan.FromSeconds(60));
            originalConnection.Settings.Properties = new Fields();
            originalConnection.Settings.Properties.Add("MyProp", "MyPropValue");
            AmqpConnection newConnection = null;

            try
            {
                // Test createing a new connection with the settings obtained while original connection is still active.
                connectionSettings = originalConnection.CreateSettingsForRecovery();
                Assert.Null(connectionSettings.RemoteContainerId);
                Assert.Null(connectionSettings.RemoteHostName);
                newConnection = await AmqpConnection.Factory.OpenConnectionAsync(addressUri, connectionSettings, AmqpConstants.DefaultTimeout);
                Assert.Equal(originalConnection.Settings.ContainerId, newConnection.Settings.ContainerId);
                Assert.Equal(originalConnection.Settings.HostName, newConnection.Settings.HostName);
                Assert.Equal("MyPropValue", newConnection.Settings.Properties["MyProp"]);
                await newConnection.OpenSessionAsync(); // verify the connection is usable.
                await newConnection.CloseAsync();

                // Test createing a new connection with the settings obtained, while original connection is already closed.
                await originalConnection.CloseAsync();
                connectionSettings = originalConnection.CreateSettingsForRecovery();
                Assert.Null(connectionSettings.RemoteContainerId);
                Assert.Null(connectionSettings.RemoteHostName);
                newConnection = await AmqpConnection.Factory.OpenConnectionAsync(addressUri, connectionSettings, AmqpConstants.DefaultTimeout);
                Assert.Equal(originalConnection.Settings.ContainerId, newConnection.Settings.ContainerId);
                Assert.Equal(originalConnection.Settings.HostName, newConnection.Settings.HostName);
                Assert.Equal("MyPropValue", newConnection.Settings.Properties["MyProp"]);
                await newConnection.OpenSessionAsync(); // verify the connection is usable.
            }
            finally
            {
                newConnection?.Close();
            }
        }

        // Test recovering a sender link by using an existing link terminus and verify that the link settings are still the same.
        [Fact]
        public async Task SenderRecoveryTest()
        {
            AmqpConnection connection = null;
            try
            {
                connection = await OpenTestConnectionAsync(addressUri);
                AmqpSession session = await connection.OpenSessionAsync();
                SendingAmqpLink originalSender = await session.OpenLinkAsync<SendingAmqpLink>(nameof(SenderRecoveryTest) + Guid.NewGuid().ToString(), nameof(SenderRecoveryTest));
                originalSender.Settings.AddProperty("MyProp", "MyPropValue");
                AmqpMessage[] messages = CreateMessages();
                foreach (AmqpMessage m in messages)
                {
                    originalSender.UnsettledMap.Add(m.DeliveryTag, m);
                }

                await originalSender.CloseAsync();

                // verrify that the link terminus object has captured the unsettled messages.
                AmqpLinkTerminus linkTerminus = originalSender.Terminus;
                foreach (AmqpMessage m in messages)
                {
                    linkTerminus.UnsettledMap.TryGetValue(m.DeliveryTag, out Delivery delivery);
                    Assert.Equal(m, delivery);
                }

                SendingAmqpLink newSender = await session.RecoverLinkAsync<SendingAmqpLink>(originalSender);
                Assert.Equal(originalSender.Name, newSender.Name);
                Assert.Equal(originalSender.IsReceiver, newSender.IsReceiver);
                Assert.Equal("MyPropValue", newSender.Settings.Properties["MyProp"]);

                // verify that sending works with this recovered link
                ReceivingAmqpLink testReceiver = await session.OpenLinkAsync<ReceivingAmqpLink>(nameof(SenderRecoveryTest) + Guid.NewGuid().ToString(), nameof(SenderRecoveryTest));
                await newSender.SendMessageAsync(AmqpMessage.Create("Hello World!"));
                Assert.NotNull(await testReceiver.ReceiveMessageAsync(TimeSpan.FromMilliseconds(5000)));
            }
            finally
            {
                connection?.Close();
            }
        }

        // Test recovering a receiver link by using an existing link terminus and verify that the link settings are still the same.
        [Fact]
        public async Task ReceiverRecoveryTest()
        {
            AmqpConnection connection = null;
            try
            {
                connection = await OpenTestConnectionAsync(addressUri);
                AmqpSession session = await connection.OpenSessionAsync();
                ReceivingAmqpLink originalReceiver = await session.OpenLinkAsync<ReceivingAmqpLink>(nameof(ReceiverRecoveryTest) + Guid.NewGuid().ToString(), nameof(ReceiverRecoveryTest));
                originalReceiver.Settings.AddProperty("MyProp", "MyPropValue");
                originalReceiver.Settings.SettleType = SettleMode.SettleOnDispose;
                AmqpMessage[] messages = CreateMessages();
                foreach (AmqpMessage m in messages)
                {
                    originalReceiver.UnsettledMap.Add(m.DeliveryTag, m);
                }

                await originalReceiver.CloseAsync();

                // verify that the link terminus object has captured the unsettled messages.
                AmqpLinkTerminus linkTerminus = originalReceiver.Terminus;
                foreach (AmqpMessage m in messages)
                {
                    linkTerminus.UnsettledMap.TryGetValue(m.DeliveryTag, out Delivery delivery);
                    Assert.Equal(m, delivery);
                }

                ReceivingAmqpLink newReceiver = await session.RecoverLinkAsync<ReceivingAmqpLink>(originalReceiver);
                Assert.Equal(originalReceiver.Name, newReceiver.Name);
                Assert.Equal(originalReceiver.IsReceiver, newReceiver.IsReceiver);
                Assert.Equal("MyPropValue", newReceiver.Settings.Properties["MyProp"]);
                Assert.Equal(originalReceiver.Settings.SettleType, newReceiver.Settings.SettleType);

                // verify that receiving and accepting works with this recovered link
                SendingAmqpLink testSender = await session.OpenLinkAsync<SendingAmqpLink>(nameof(ReceiverRecoveryTest) + Guid.NewGuid().ToString(), nameof(ReceiverRecoveryTest));
                await testSender.SendMessageAsync(AmqpMessage.Create("Hello World2!"));
                AmqpMessage received = await newReceiver.ReceiveMessageAsync(TimeSpan.FromMilliseconds(5000));
                Assert.NotNull(received);
                newReceiver.AcceptMessage(received);
            }
            finally
            {
                connection?.Close();
            }
        }

        // Test opening new two senders using the same link name without EnableLinkRecovery. Should be able to create both.
        [Fact]
        public async Task LinkRecoveryNotEnabledSendersNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(false, true, false, false);
        }

        // Test opening new two receivers using the same link name without EnableLinkRecovery. Should be able to create both.
        [Fact]
        public async Task LinkRecoveryNotEnabledReceiversNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(false, true, false, false);
        }

        // Test opening two new senders using the same link name with EnableLinkRecovery. Should throw exception upon opening the second one.
        [Fact]
        public async Task NewSendersNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(true, true, false, false);
        }

        // Test opening two new receivers using the same link name with EnableLinkRecovery. Should throw exception upon opening the second one.
        [Fact]
        public async Task NewReceiversNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(true, true, false, false);
        }

        // Test opening a sender then recovering another sender using the same link name with EnableLinkRecovery. Should throw exception upon opening the second one.
        [Fact]
        public async Task RecoveringSenderNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(true, false, false, false);
        }

        // Test opening a sender then recovering another receiver using the same link name with EnableLinkRecovery. Should throw exception upon opening the second one.
        [Fact]
        public async Task RecoveringReceiverNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(true, false, false, false);
        }

        // Test opening a sender then a receiver using the same link name with EnableLinkRecovery. Should be able to create both.
        [Fact]
        public async Task SenderAndReceiverNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, ReceivingAmqpLink>(true, true, false, false);
        }

        // Test opening and closing a sender then opening another sender using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task CreateNewSenderAfterClosedSenderNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(true, true, true, false);
        }

        // Test opening and closing a receiver then opening another receiver using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task CreateNewReceiverAfterClosedReceiverNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(true, true, true, false);
        }

        // Test opening and aborting a sender then opening another sender using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task CreateNewSenderAfterAbortedSenderNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(true, true, false, true);
        }

        // Test opening and aborting a receiver then opening another receiver using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task CreateNewReceiverAfterAbortedReceiverNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(true, true, false, true);
        }

        // Test opening and closing a sender then recovering another sender using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task ReceoverSenderAfterClosedSenderNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(true, false, true, false);
        }

        // Test opening and closing a receiver then recovering another receiver using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task RecoverReceiverAfterClosedReceiverNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(true, false, true, false);
        }

        // Test opening and aborting a sender then opening another sender using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task RecoverSenderAfterAbortedSenderNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<SendingAmqpLink, SendingAmqpLink>(true, false, false, true);
        }

        // Test opening and aborting a receiver then recovering another receiver using the same link name with EnableLinkRecovery. Should be able to create the second one.
        [Fact]
        public async Task RecoverReceiverAfterAbortedReceiverNameUniquenessTest()
        {
            await LinkTerminusUniquenessTestAsync<ReceivingAmqpLink, ReceivingAmqpLink>(true, false, false, true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 1.
        // Local sender has DeliveryState = null, remote receiver does not have this unsettled delivery.
        // Expected behavior is that the sender will immediately resend this delivery with field resume=false if settle mode is not settle-on-send.
        [Fact]
        public async Task ClientSenderNullDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderNullDeliveryStateBrokerNoDeliveryStateTest),
                true,
                null,
                false,
                null,
                true,
                testSettleOnSend: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 1 with sender/receiver swapped.
        // Local receiver has DeliveryState = null, remote sender does not have this unsettled delivery.
        // Expected behavior is that the sender will not be sending anything, the client side receiver should just remove this unsettled delivery.
        [Fact]
        public async Task ClientReceiverNullDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverNullDeliveryStateBrokerNoDeliveryStateTest),
                true,
                null,
                false,
                null,
                false);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 2
        // Local sender has DeliveryState = null, remote receiver has DeliveryState = Received.
        // Expected behavior is that the sender will immediately resend this delivery from the start with field resume=true.
        [Fact]
        public async Task ClientSenderNullDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderNullDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                null,
                true,
                AmqpConstants.ReceivedOutcome,
                true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 2 with sender/receiver swapped. This is essentially the same as example delivery tag 9.
        // Local receiver has DeliveryState = null, remote sender has DeliveryState = Received.
        // Expected behavior is that the sender will immediately resend this delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientReceiverNullDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverNullDeliveryStateBrokerNoDeliveryStateTest),
                true,
                null,
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 3.
        // Local sender has DeliveryState = null, remote receiver has reached terminal DeliveryState.
        // Expected behavior is that the sender will just settle the delivery locally with nothing being sent.
        [Fact]
        public async Task ClientSenderNullDeliveryStateBrokerTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderNullDeliveryStateBrokerTerminalDeliveryStateTest),
                true,
                null,
                true,
                AmqpConstants.AcceptedOutcome,
                false);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 3 with sender/receiver swapped. This is essentially the same as example delivery tag 14.
        // Local receiver has DeliveryState = null, remote sender has terminal DeliveryState.
        // Expected behavior is that the sender will resend the delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientReceiverNullDeliveryStateBrokerTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverNullDeliveryStateBrokerTerminalDeliveryStateTest),
                true,
                null,
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 4.
        // Local sender has DeliveryState = null, remote receiver has DeliveryState = null.
        // Expected behavior is that the sender will immediately resend this delivery with field resume=true if settle mode is not settle-on-send.
        [Fact]
        public async Task ClientSenderNullDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderNullDeliveryStateBrokerNullDeliveryStateTest),
                true,
                null,
                true,
                null,
                true,
                testSettleOnSend: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 4 with sender/receiver swapped. This is essentially the same as example delivery tag 14.
        // Local receiver has DeliveryState = null, remote sender has DeliveryState = null.
        // Expected behavior is that the sender will immediately resend this delivery with field resume=true if settle mode is not settle-on-send.
        [Fact]
        public async Task ClientReceiverNullDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverNullDeliveryStateBrokerNullDeliveryStateTest),
                true,
                null,
                true,
                null,
                true,
                testSettleOnSend: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 5.
        // Local sender has DeliveryState = Received, remote receiver DeliveryState does not exist.
        // Expected behavior is that the sender will immediately resend this delivery with field resume=false if settle mode is not settle-on-send.
        [Fact]
        public async Task ClientSenderReceivedDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderReceivedDeliveryStateBrokerNoDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                false,
                null,
                true,
                testSettleOnSend: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 5 with sender/receiver swapped.
        // Local receiver has DeliveryState = Received, remote sender DeliveryState does not exist.
        // Expected behavior is that the sender will not be sending anything, the client side receiver should just remove this unsettled delivery.
        [Fact]
        public async Task ClientReceiverNoDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverNoDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                false,
                null,
                false);

        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 6, 7.
        // Local sender has DeliveryState = Received, remote receiver has DeliveryState = Received.
        // Expected behavior is that the sender will immediately resend this delivery from the start with field resume=true and aborted=true.
        [Fact]
        public async Task ClientSenderReceivedDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderReceivedDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 6, 7 with sender/receiver swapped.
        // Local receiver has DeliveryState = Received, remote sender has DeliveryState = Received.
        // Expected behavior is that the sender will immediately resend this delivery from the start with field resume=true and aborted=true.
        [Fact]
        public async Task ClientReceiverReceivedDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverReceivedDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 8.
        // Local sender has DeliveryState = Received, remote receiver has reached terminal outcome.
        // Expected behavior is that the sender will just settle the delivery locally without resending the delivery.
        [Fact]
        public async Task ClientSenderReceivedDeliveryStateBrokerTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderReceivedDeliveryStateBrokerTerminalDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                AmqpConstants.AcceptedOutcome,
                false);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 8 with sender/receiver swapped. This is essentially the same as example delivery tag 11.
        // Local receiver has DeliveryState = Received, remote sender has reached terminal outcome.
        // Expected behavior is that the sender will resend the delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientReceiverReceivedDeliveryStateBrokerTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverReceivedDeliveryStateBrokerTerminalDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 9.
        // Local sender has DeliveryState = Received, remote receiver has DeliveryState = null.
        // Expected behavior is that the sender will immediately resend this delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientSenderReceivedDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderReceivedDeliveryStateBrokerNullDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                null,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 9 with sender/receiver swapped. This is essentially the same as example delivery tag 2.
        // Local receiver has DeliveryState = Received, remote sender has DeliveryState = null.
        // Expected behavior is that the sender will immediately resend this delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientReceiverReceivedDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverReceivedDeliveryStateBrokerNullDeliveryStateTest),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                null,
                true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 10
        // Local sender has terminal DeliveryState, remote receiver does not have this DeliveryState.
        // Expected behavior is that the sender will just settle the delivery locally without resending the delivery.
        [Fact]
        public async Task ClientSenderTerminalDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalDeliveryStateBrokerNoDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                false,
                null,
                false);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 10 with sender/receiver swapped.
        // Local receiver has terminal DeliveryState, remote sender does not have this DeliveryState.
        // Expected behavior is that the sender will not be sending anything, the client side receiver should just remove this unsettled delivery.
        [Fact]
        public async Task ClientReceiverTerminalDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalDeliveryStateBrokerNoDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                false,
                null,
                false);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 11
        // Local sender has terminal DeliveryState, remote receiver has DeliveryState = Received.
        // Expected behavior is that the sender will resend the delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientSenderTerminalDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 11 with sender/receiver swapped. This is essentially the same as example delivery tag 8.
        // Local receiver has terminal DeliveryState, remote sender has DeliveryState = Received.
        // Expected behavior is that the sender will just settle the delivery locally without resending the delivery.
        [Fact]
        public async Task ClientReceiverTerminalDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                AmqpConstants.ReceivedOutcome,
                false);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 12
        // Local sender has terminal DeliveryState, remote receiver has the same terminal DeliveryState.
        // Expected behavior is that the sender will resend the delivery with resume=true to settle the delivery.
        [Fact]
        public async Task ClientSenderTerminalDeliveryStateBrokerSameTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalDeliveryStateBrokerSameTerminalDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                AmqpConstants.AcceptedOutcome,
                true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 12 with sender/receiver swapped. 
        // Local receiver has terminal DeliveryState, remote sender has the same terminal DeliveryState.
        // Expected behavior is that the sender will resend the delivery with resume=true to settle the delivery.
        [Fact]
        public async Task ClientReceiverTerminalDeliveryStateBrokerSameTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalDeliveryStateBrokerSameTerminalDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                AmqpConstants.AcceptedOutcome,
                true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 13
        // Local sender has terminal DeliveryState, remote receiver has the different terminal DeliveryState.
        // Expected behavior is that the sender will resend the delivery with resume=true and DeliveryState equal to the sender's DeliveryState to settle the delivery.
        [Fact]
        public async Task ClientSenderTerminalDeliveryStateBrokerDiffTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalDeliveryStateBrokerDiffTerminalDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                AmqpConstants.RejectedOutcome,
                true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 13 with sender/receiver swapped.
        // Local receiver has terminal DeliveryState, remote sender has the different terminal DeliveryState.
        // Expected behavior is that the sender will resend the delivery with resume=true and DeliveryState equal to the sender's DeliveryState to settle the delivery.
        [Fact]
        public async Task ClientReceiverTerminalDeliveryStateBrokerDiffTerminalDeliveryStateTest()
        {
            // Note: This test will actually fail if Released state is used instead of Rejected,
            // because broker will interpret it as actually releasing the lock on the message,
            // and resend the delivery again to the next available consumer, which is this test link (for a third time).
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalDeliveryStateBrokerDiffTerminalDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                AmqpConstants.RejectedOutcome,
                true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 14.
        // Local sender has terminal DeliveryState, remote receiver has null DeliveryState.
        // Expected behavior is that the sender will resend the delivery with resume=true and aborted=true.
        [Fact]
        public async Task ClientSenderTerminalDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalDeliveryStateBrokerNullDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                null,
                true,
                shouldAbortDelivery: true);
        }

        // Oasis AMQP doc section 3.4.6, example delivery tag 14 with sender/receiver swapped. This is essentially the same as example delivery tag 3.
        // Local receiver has terminal DeliveryState, remote sender has null DeliveryState.
        // Expected behavior is that the sender will just settle the delivery locally with nothing being sent.
        [Fact]
        public async Task ClientReceiverTerminalDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalDeliveryStateBrokerNullDeliveryStateTest),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                null,
                false);
        }

        // Test when local sender is in pending transactional delivery state and remote has no record of this delivery.
        // Expected behavior is that the sender should resend the message if settle mode is not settle-on-send, similar to Oasis AMQP doc section 3.4.6, example delivery tag 1.
        [Fact]
        public async Task ClientSenderTransactionalDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTransactionalDeliveryStateBrokerNoDeliveryStateTest),
                true,
                new TransactionalState(),
                false,
                null,
                true,
                testSettleOnSend: true);
        }

        // Test when local sender is in pending transactional delivery state and remote has DeliveryState = null.
        // Expected behavior is that the sender should abort the delivery because we are unsure of the sender's state of delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 9.
        [Fact]
        public async Task ClientSenderTransactionalDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTransactionalDeliveryStateBrokerNullDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                null,
                true,
                shouldAbortDelivery: true);
        }

        // Test when local sender is in pending transactional delivery state and remote has DeliveryState = null.
        // Expected behavior is that the sender should abort the delivery because we are unsure of the sender's state of delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 9.
        [Fact]
        public async Task ClientSenderTransactionalDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTransactionalDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Test when local sender is in pending transactional delivery state and remote has reached non-transactional terminal state.
        // Expected behavior is that the sender should abort the delivery because the receiver should not have been able to become non-transactional.
        [Fact]
        public async Task ClientSenderTransactionalDeliveryStateBrokerTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTransactionalDeliveryStateBrokerTerminalDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Test when local sender and remote receiver are both in pending transactional delivery state.
        // Expected behavior is that the sender should abort the delivery because we are unsure of the sender's state of delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 7.
        [Fact]
        public async Task ClientSenderTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest()
        {
            var txnId = new ArraySegment<byte>(Guid.NewGuid().ToByteArray());
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTransactionalDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                new TransactionalState(),
                true,
                shouldAbortDelivery: true);
        }

        // Test when local receiver is in pending transactional delivery state and remote sender is in terminal transactional delivery state.
        // Expected behavior is that the sender should abort the delivery because we are unsure of the sender's state of delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 3, 8.
        [Fact]
        public async Task ClientSenderTransactionalDeliveryStateBrokerTerminalTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTransactionalDeliveryStateBrokerTerminalTransactionalDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                false);
        }

        // Test when local sender is in terminal transactional delivery state and remote receiver does not have this delivery.
        // Expected behavior is that the sender should not resend any deliveries because the receiver must have already processed and settled this delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 10.
        [Fact]
        public async Task ClientSenderTerminalTransactionalDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalTransactionalDeliveryStateBrokerNoDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                false,
                null,
                false);
        }

        // Test when local sender is in terminal transactional delivery state and remote receiver is in pending transactional delivery state.
        // Expected behavior is that the sender should abort the delivery because the sender cannot resume the delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 11, 14.
        [Fact]
        public async Task ClientSenderTerminalTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                new TransactionalState(),
                true,
                shouldAbortDelivery: true);
        }

        // Test when local sender and remote receiver are both in the same terminal transactional state.
        // Expected behavior is that the sender should send a delivery to settle the delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 12.
        [Fact]
        public async Task ClientSenderTerminalTransactionalDeliveryStateBrokerSameTerminalTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalTransactionalDeliveryStateBrokerSameTerminalTransactionalDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true);
        }

        // Test when local sender and remote receiver are both in the different terminal transactional states.
        // Expected behavior is that the sender should send a delivery with the sender's delivery states to settle the delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 13.
        [Fact]
        public async Task ClientSenderTerminalTransactionalDeliveryStateBrokerDiffTerminalTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<SendingAmqpLink>(
                nameof(ClientSenderTerminalTransactionalDeliveryStateBrokerDiffTerminalTransactionalDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                new TransactionalState() { Outcome = AmqpConstants.RejectedOutcome },
                true);
        }

        // Test when local receiver is in pending transactional delivery state and remote has no record of this delivery.
        // Expected behavior is that the should not be sending anything because it has no record of this delivery to send.
        [Fact]
        public async Task ClientReceiverTransactionalDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTransactionalDeliveryStateBrokerNoDeliveryStateTest),
                true,
                new TransactionalState(),
                false,
                null,
                false);
        }

        // Test when local receiver is in pending transactional delivery state and remote has DeliveryState = null.
        // Expected behavior is that the sender should abort the delivery because the local receiver should not have been transactional.
        [Fact]
        public async Task ClientReceiverTransactionalDeliveryStateBrokerNullDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTransactionalDeliveryStateBrokerNullDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                null,
                true,
                shouldAbortDelivery: true);
        }

        // Test when local receiver is in pending transactional delivery state and remote has DeliveryState = Received.
        // Expected behavior is that the sender should abort the delivery because the local receiver should not have been transactional.
        [Fact]
        public async Task ClientReceiverTransactionalDeliveryStateBrokerReceivedDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTransactionalDeliveryStateBrokerReceivedDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                AmqpConstants.ReceivedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Test when local receiver is in pending transactional delivery state and remote has terminal non-transactional delivery state.
        // Expected behavior is that the sender should abort the delivery because the local receiver should not have been transactional.
        [Fact]
        public async Task ClientReceiverTransactionalDeliveryStateBrokerTerminalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTransactionalDeliveryStateBrokerTerminalDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                AmqpConstants.AcceptedOutcome,
                true,
                shouldAbortDelivery: true);
        }

        // Test when local sender and remote receiver are both in pending transactional delivery state.
        // Expected behavior is that the sender should abort the delivery because we are unsure of the sender's state of delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 7.
        [Fact]
        public async Task ClientReceiverTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                new TransactionalState(),
                true,
                shouldAbortDelivery: true);
        }

        // Test when local receiver is in pending transactional delivery state and remote sender is in terminal transactional delivery state.
        // Expected behavior is that the sender should abort the delivery because we are unsure of the sender's state of delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 11, 14.
        [Fact]
        public async Task ClientReceiverTransactionalDeliveryStateBrokerTerminalTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTransactionalDeliveryStateBrokerTerminalTransactionalDeliveryStateTest),
                true,
                new TransactionalState(),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                shouldAbortDelivery: true);
        }

        // Test when local receiver is in terminal transactional delivery state and remote sender is in pending transactional delivery state.
        // Expected behavior is that the sender should not be sending anything because the receiver has already reached terminal state.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 3, 8.
        [Fact]
        public async Task ClientReceiverTerminalTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalTransactionalDeliveryStateBrokerTransactionalDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                new TransactionalState(),
                false);
        }

        // Test when local receiver is in terminal transactional delivery state and remote sender does not have this delivery.
        // Expected behavior is that the sender should not be sending anything because the receiver has already reached terminal state.
        [Fact]
        public async Task ClientReceiverTerminalTransactionalDeliveryStateBrokerNoDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalTransactionalDeliveryStateBrokerNoDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                false,
                null,
                false);
        }

        // Test when local receiver and remote sender are both in the same terminal transactional state.
        // Expected behavior is that the sender should send a delivery to settle the delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 12.
        [Fact]
        public async Task ClientReceiverTerminalTransactionalDeliveryStateBrokerSameTerminalTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalTransactionalDeliveryStateBrokerSameTerminalTransactionalDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true);
        }

        // Test when local receiver and remote sender are both in the same terminal transactional state.
        // Expected behavior is that the sender should send a delivery with the sender's delivery states to settle the delivery.
        // Similar to Oasis AMQP doc section 3.4.6, example delivery tag 13.
        [Fact]
        public async Task ClientReceiverTerminalTransactionalDeliveryStateBrokerDiffTerminalTransactionalDeliveryStateTest()
        {
            await NegotiateUnsettledDeliveryTestAsync<ReceivingAmqpLink>(
                nameof(ClientReceiverTerminalTransactionalDeliveryStateBrokerDiffTerminalTransactionalDeliveryStateTest),
                true,
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                true,
                new TransactionalState() { Outcome = AmqpConstants.RejectedOutcome },
                true);
        }

        [Fact]
        public async Task CannotRecoverOpenLinkTest()
        {
            await CannotRecoverOpenLinkTestCase<SendingAmqpLink>();
            await CannotRecoverOpenLinkTestCase<ReceivingAmqpLink>();
        }

        async Task CannotRecoverOpenLinkTestCase<T>() where T : AmqpLink
        {
            string linkName = Guid.NewGuid().ToString();
            AmqpConnection connection = await OpenTestConnectionAsync(addressUri);
            AmqpConnection brokerConnection = broker.FindConnection(connection.Settings.ContainerId);
            try
            {
                AmqpSession session = await connection.OpenSessionAsync();
                var localLink = await session.OpenLinkAsync<T>(linkName, addressUri.AbsoluteUri);
                
                // should not be able to recover a still open link because the link terminus info with the unsettled map is captured during close.
                await Assert.ThrowsAsync<InvalidOperationException>(() => session.RecoverLinkAsync<T>(localLink));
                
                // close the link and recover it now, should work.
                await localLink.CloseAsync();
                var newLocalLink = await session.RecoverLinkAsync<T>(localLink);

                // the recovered link should be blocked from recovering as well when it's still open.
                await Assert.ThrowsAsync<InvalidOperationException>(() => session.RecoverLinkAsync<T>(newLocalLink));

                // try closing the link from the remote side. Should be able to recover it now.
                brokerConnection.RecoverableLinkEndpoints.TryGetValue(new AmqpLinkSettings() { LinkName = localLink.Name, Role = !localLink.Settings.Role }, out AmqpLink brokerlink);
                await brokerlink.CloseAsync();
                newLocalLink = await session.RecoverLinkAsync<T>(localLink);
            }
            finally
            {
                await connection.CloseAsync();
            }
        }

        /// <summary>
        /// Test if the terminus uniqueness under a connection would be enforced given links with the same link names.
        /// </summary>
        /// <typeparam name="T1">The type of link1.</typeparam>
        /// <typeparam name="T2">The type of link2.</typeparam>
        /// <param name="linkRecoveryEnabled">True if EnableLinkRecovery is true for the connection.</param>
        /// <param name="openNewLink">True if the second link should be a newly opened one. False if the second link should be recovered with a link terminus.</param>
        /// <param name="shouldClose">True if the first link should be closed before opening the second one.</param>
        /// <param name="shouldAbort">True if the first link should be aborted before opening the second one.</param>
        /// <returns></returns>
        static async Task LinkTerminusUniquenessTestAsync<T1, T2>(bool linkRecoveryEnabled, bool openNewLink, bool shouldClose, bool shouldAbort)
            where T1 : AmqpLink
            where T2 : AmqpLink
        {
            string linkName = Guid.NewGuid().ToString();
            AmqpConnection connection = linkRecoveryEnabled ? await OpenTestConnectionAsync(addressUri) : await AmqpConnection.Factory.OpenConnectionAsync(addressUri);
            try
            {
                AmqpSession recoverableSession1 = await connection.OpenSessionAsync(new AmqpSessionSettings());
                AmqpSession recoverableSession2 = await connection.OpenSessionAsync(new AmqpSessionSettings());
                T1 link1 = await recoverableSession1.OpenLinkAsync<T1>(linkName, addressUri.AbsoluteUri);

                if (shouldClose)
                {
                    await link1.CloseAsync();
                }
                else if (shouldAbort)
                {
                    link1.Abort();
                }

                bool shouldThrow = linkRecoveryEnabled && !shouldClose && !shouldAbort && typeof(T1) == typeof(T2);
                Task openLink2Task = openNewLink ? recoverableSession2.OpenLinkAsync<T2>(linkName, addressUri.AbsoluteUri) : recoverableSession2.RecoverLinkAsync<T2>(link1);
                if (shouldThrow)
                {
                    await Assert.ThrowsAsync<InvalidOperationException>(() => openLink2Task);
                }
                else
                {
                    await openLink2Task;
                }
            }
            finally
            {
                connection.Close();
            }
        }

        /// <summary>
        /// Test the negotiation of a single unsettled delivery between local and the remote peer.
        /// Please see the OASIS AMQP doc section 3.4.6 for the test scenarios.
        /// </summary>
        /// <typeparam name="T">The type of link that the local side will open towards remote (sending or receiving).</typeparam>
        /// <param name="testName">The name of the test. This will be used to set the link name as well as the queue name used during this test.</param>
        /// <param name="hasLocalDeliveryState">True if the local link unsettled map should have record of the unsettled delivery.</param>
        /// <param name="localDeliveryState">The actual value of the local unsettled delivery state.</param>
        /// <param name="hasRemoteDeliveryState">True if the remote link unsettled map should have record of the unsettled delivery.</param>
        /// <param name="remoteDeliveryState">The actual value of the local unsettled delivery state.</param>
        /// <param name="expectSend">True if the sender is expected to resend the unsettled delivery after negotiation with the receiver unsettled map.</param>
        /// <param name="shouldAbortDelivery">True if the delivery sent by the sender should have the "Aborted" field set.</param>
        /// <param name="testSettleOnSend">True if the same test should be run again with link.SettleType = SettleMode.SettleOnSend (default is SettleMode.SettleOnReceive).</param>
        /// <returns></returns>
        static async Task NegotiateUnsettledDeliveryTestAsync<T>(
            string testName,
            bool hasLocalDeliveryState,
            DeliveryState localDeliveryState,
            bool hasRemoteDeliveryState,
            DeliveryState remoteDeliveryState,
            bool expectSend,
            bool shouldAbortDelivery = false,
            bool testSettleOnSend = false) where T : AmqpLink
        {
            TestAmqpConnection connection = await OpenTestConnectionAsync(addressUri);
            try
            {
                TestAmqpConnection brokerConnection = broker.FindConnection(connection.Settings.ContainerId) as TestAmqpConnection;
                TestAmqpConnection receiverSideConnection = typeof(T) == typeof(SendingAmqpLink) ? brokerConnection : connection;
                AmqpSession session = await connection.OpenSessionAsync();

                // If needed, actually declare the transaction so the broker can find this transaction and not throw exceptions.
                Controller txController = null;
                ArraySegment<byte> txnId = default;
                if (localDeliveryState is TransactionalState || remoteDeliveryState is TransactionalState)
                {
                    DeclareTransaction(session, localDeliveryState, remoteDeliveryState, out txController, out txnId);
                }

                // Set up the unsettled message for both the local unsettled map and the remote unsettled map.
                var unsettledMap = new Dictionary<ArraySegment<byte>, Delivery>(ByteArrayComparer.Instance);
                var deliveryTag = new ArraySegment<byte>(Guid.NewGuid().ToByteArray());
                AmqpMessage localUnsettledMessage = hasLocalDeliveryState ? AddClientUnsettledDelivery(unsettledMap, deliveryTag, localDeliveryState) : null;
                AmqpMessage remoteUnsettledMessage = hasRemoteDeliveryState ? AddBrokerUnsettledDelviery($"{testName}1", deliveryTag, remoteDeliveryState) : null;
                AmqpMessage senderSideUnsettledMessage = typeof(T) == typeof(SendingAmqpLink) ? localUnsettledMessage : remoteUnsettledMessage;
                AmqpMessage receiverSideUnsettledMessage = typeof(T) == typeof(SendingAmqpLink) ? remoteUnsettledMessage : localUnsettledMessage;

                var localLink = await OpenTestLinkAsync<T>(session, $"{testName}1", unsettledMap);
                Transfer expectedTransfer = receiverSideConnection.ReceivedPerformatives.Last.Value as Transfer;
                bool transferSettled = expectedTransfer?.Settled == true;
                bool shouldSetResumeFlag = typeof(T) == typeof(SendingAmqpLink) ? hasRemoteDeliveryState : hasLocalDeliveryState;

                if (expectSend)
                {
                    Assert.NotNull(expectedTransfer);
                    Assert.Equal(expectedTransfer.Resume, shouldSetResumeFlag);
                    Assert.Equal(expectedTransfer.Aborted, shouldAbortDelivery);
                    Outcome localDeliveryOutcome = localDeliveryState?.Outcome();
                    Outcome remoteDeliveryOutcome = remoteDeliveryState?.Outcome();
                    Assert.Equal(localDeliveryOutcome != null && remoteDeliveryOutcome != null && localDeliveryOutcome.GetType() == remoteDeliveryOutcome.GetType(), transferSettled);

                    if (txController != null)
                    {
                        await txController.DischargeAsync(txnId, false);
                    }

                    if (typeof(T) == typeof(SendingAmqpLink))
                    {
                        var testDummyReceiver = await session.OpenLinkAsync<ReceivingAmqpLink>($"{testName}1-testReceiver", $"{testName}1");
                        await TestReceivingMessageAsync(testDummyReceiver, transferSettled || shouldAbortDelivery ? null : senderSideUnsettledMessage);
                    }
                    else
                    {
                        // If the transfer was already aborted or settled, then the receiving link is expected to not process this transfer, therefore not receiving any message.
                        await TestReceivingMessageAsync(localLink as ReceivingAmqpLink, transferSettled || shouldAbortDelivery ? null : senderSideUnsettledMessage);
                    }
                }
                else
                {
                    if (!(receiverSideConnection.ReceivedPerformatives.Last.Value is Attach))
                    {
                        Console.WriteLine("a");
                    }
                    Assert.True(receiverSideConnection.ReceivedPerformatives.Last.Value is Attach);
                    if (typeof(T) == typeof(SendingAmqpLink))
                    {
                        var receiver = await session.OpenLinkAsync<ReceivingAmqpLink>($"{testName}1-testReceiver", $"{testName}1");
                        await TestReceivingMessageAsync(receiver, null);
                    }
                    else
                    {
                        await TestReceivingMessageAsync(localLink as ReceivingAmqpLink, null);
                    }
                }

                await localLink.CloseAsync();
                if (testSettleOnSend)
                {
                    if (localDeliveryState is TransactionalState || remoteDeliveryState is TransactionalState)
                    {
                        DeclareTransaction(session, localDeliveryState, remoteDeliveryState, out txController, out txnId);
                    }

                    // When settle mode is SettleMode.SettleOnSend, the client sender does not need to resend the message upon open.
                    unsettledMap = new Dictionary<ArraySegment<byte>, Delivery>(ByteArrayComparer.Instance);
                    deliveryTag = new ArraySegment<byte>(Guid.NewGuid().ToByteArray());
                    localUnsettledMessage = hasLocalDeliveryState ? AddClientUnsettledDelivery(unsettledMap, deliveryTag, localDeliveryState) : null;
                    remoteUnsettledMessage = hasRemoteDeliveryState ? AddBrokerUnsettledDelviery(testName, deliveryTag, remoteDeliveryState) : null;

                    localLink = await OpenTestLinkAsync<T>(session, $"{testName}2", unsettledMap, SettleMode.SettleOnSend);
                    Assert.True(receiverSideConnection.ReceivedPerformatives.Last.Value is Attach);

                    if (txController != null)
                    {
                        await txController.DischargeAsync(txnId, false);
                    }

                    if (typeof(T) == typeof(SendingAmqpLink))
                    {
                        var testDummyreceiver = await session.OpenLinkAsync<ReceivingAmqpLink>($"{testName}2-testReceiver", $"{testName}2");
                        await TestReceivingMessageAsync(testDummyreceiver, null);
                    }
                    else
                    {
                        await TestReceivingMessageAsync(localLink as ReceivingAmqpLink, null);
                    }
                }
            }
            finally
            {
                connection?.Close();
            }
        }

        static async Task<TestAmqpConnection> OpenTestConnectionAsync(Uri addressUri)
        {
            AmqpConnectionFactory factory = new AmqpConnectionFactory();
            AmqpSettings settings = factory.GetAmqpSettings(null);
            TransportBase transport = await factory.GetTransportAsync(addressUri, settings, AmqpConstants.DefaultTimeout, CancellationToken.None);
            var connection = new TestAmqpConnection(transport, settings, new AmqpConnectionSettings() { ContainerId = Guid.NewGuid().ToString(), HostName = addressUri.Host, EnableLinkRecovery = true });
            await connection.OpenAsync();
            return connection;
        }

        static AmqpMessage AddClientUnsettledDelivery(Dictionary<ArraySegment<byte>, Delivery> unsettledMap, ArraySegment<byte> deliveryTag, DeliveryState deliveryState)
        {
            AmqpMessage message = AmqpMessage.Create("My Message");
            message.DeliveryTag = deliveryTag;
            message.State = deliveryState;
            unsettledMap.Add(message.DeliveryTag, message);
            return message;
        }

        static AmqpMessage AddBrokerUnsettledDelviery(string linkName, ArraySegment<byte> deliveryTag, DeliveryState deliveryState)
        {
            AmqpMessage message = AmqpMessage.Create("My Message");
            message.DeliveryTag = deliveryTag;
            message.State = deliveryState;
            BrokerMessage brokerMessage = new BrokerMessage(message) { DeliveryTag = deliveryTag, State = deliveryState };
            broker.MockUnsettledReceivingDeliveries.AddOrUpdate(linkName, (key) => new List<Delivery>() { brokerMessage },
                (key, unsettledDeliveries) =>
                {
                    unsettledDeliveries.Add(brokerMessage);
                    return unsettledDeliveries;
                });

            return message;
        }

        static async Task<AmqpLink> OpenTestLinkAsync<T>(AmqpSession session, string linkName, Dictionary<ArraySegment<byte>, Delivery> unsettledMap, SettleMode settleMode = SettleMode.SettleOnReceive) where T : AmqpLink
        {
            Type linkType = typeof(T);
            AmqpLinkSettings linkSettings = new AmqpLinkSettings();
            linkSettings.Unsettled = new AmqpMap(
                unsettledMap.ToDictionary(
                    kvPair => kvPair.Key,
                    kvPair => kvPair.Value.State),
                ByteArrayComparer.MapKeyByteArrayComparer.Instance);

            if (linkType == typeof(SendingAmqpLink))
            {
                linkSettings.LinkName = linkName;
                linkSettings.Role = false;
                linkSettings.Source = new Source();
                linkSettings.Target = new Target() { Address = linkName };
            }
            else if (linkType == typeof(ReceivingAmqpLink))
            {
                linkSettings.LinkName = linkName;
                linkSettings.Role = true;
                linkSettings.Source = new Source() { Address = linkName };
                linkSettings.TotalLinkCredit = AmqpConstants.DefaultLinkCredit;
                linkSettings.AutoSendFlow = true;
                linkSettings.Target = new Target();
            }
            else
            {
                throw new NotSupportedException(linkType.Name);
            }

            linkSettings.SettleType = settleMode;
            var terminus = new AmqpLinkTerminus(linkSettings, unsettledMap);
            AmqpLink mockExistingLink;
            if (typeof(T) == typeof(SendingAmqpLink))
            {
                mockExistingLink = new SendingAmqpLink(linkSettings) { Terminus = terminus };
            }
            else
            {
                mockExistingLink = new ReceivingAmqpLink(linkSettings) { Terminus = terminus };
            }

            AmqpLink link = await session.RecoverLinkAsync<T>(mockExistingLink);
            await Task.Delay(1000); // wait for the sender to potentially send the initial deliveries
            return link;
        }

        /// <summary>
        /// Try receiving the message to verify that the message was indeed sent to the broker.
        /// If <paramref name="expectedMessage"/> is null, then the receiver is expected to not receive any message.
        /// </summary>
        /// <param name="receiver">The receiver to receive the expected message with.</param>
        /// <param name="expectedMessage">The expected message to be received. Null if there should be no message received.</param>
        static async Task TestReceivingMessageAsync(ReceivingAmqpLink receiver, AmqpMessage expectedMessage)
        {
            try
            {
                AmqpMessage received = await receiver.ReceiveMessageAsync(TimeSpan.FromSeconds(2));
                if (expectedMessage == null)
                {
                    Assert.Null(received);
                }
                else
                {
                    Assert.NotNull(received);
                    Assert.Equal(expectedMessage.ValueBody.Value, received.ValueBody.Value);
                    receiver.AcceptMessage(received);
                }
            }
            finally
            {
                await receiver.CloseAsync();
            }
        }

        static AmqpMessage[] CreateMessages()
        {
            DeliveryState[] deliveryStates = new DeliveryState[] 
            { 
                null,
                AmqpConstants.ReceivedOutcome,
                AmqpConstants.AcceptedOutcome,
                AmqpConstants.RejectedOutcome,
                AmqpConstants.ReleasedOutcome,
                new Modified(),
                new TransactionalState(),
                new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome },
                new TransactionalState() { Outcome = AmqpConstants.RejectedOutcome },
                new TransactionalState() { Outcome = AmqpConstants.ReleasedOutcome },
                new TransactionalState() { Outcome = new Modified() }
            };

            AmqpMessage[] messages = new AmqpMessage[deliveryStates.Length];
            for (int i = 0; i < deliveryStates.Length; i++)
            {
                messages[i] = AmqpMessage.Create("Message" + i);
                messages[i].State = deliveryStates[i];
                messages[i].DeliveryTag = new ArraySegment<byte>(Guid.NewGuid().ToByteArray());
            }

            return messages;
        }

        static void DeclareTransaction(AmqpSession session, DeliveryState localDeliveryState, DeliveryState remoteDeliveryState, out Controller txController, out ArraySegment<byte> txnId)
        {
            Fx.Assert(localDeliveryState is TransactionalState || remoteDeliveryState is TransactionalState, "at least one delivery state needs to be transactional to declare a trnasaction for a test.");
            txController = new Controller(session, TimeSpan.FromSeconds(10));
            txController.Open();
            txnId = txController.DeclareAsync().Result;
            var localTransactionalState = localDeliveryState as TransactionalState;
            var remoteTransactionalState = remoteDeliveryState as TransactionalState;

            if (localTransactionalState != null)
            {
                localTransactionalState.TxnId = txnId;
            }

            if (remoteTransactionalState != null)
            {
                remoteTransactionalState.TxnId = txnId;
            }
        }
    }
}