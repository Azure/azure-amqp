// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Xunit;
    using TestAmqpBroker;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.Azure.Amqp;

    [Collection("AmqpLinkTests")]
    [Trait("Category", TestCategory.Current)]
    public class AmqpLinkStealingTests : IClassFixture<TestAmqpBrokerFixture>, IDisposable
    {
        static Uri connectionAddressUri;
        static TestAmqpBroker broker;
        static bool enableLinkRecovery;

        public AmqpLinkStealingTests(TestAmqpBrokerFixture testAmqpBrokerFixture)
        {
            connectionAddressUri = TestAmqpBrokerFixture.Address;
            broker = testAmqpBrokerFixture.Broker;
            if (enableLinkRecovery)
            {
                broker.SetTerminusStore(new AmqpInMemoryTerminusStore());
            }
        }

        // This would be run after each test case.
        public void Dispose()
        {
            broker.SetTerminusStore(null);
        }

        /// <summary>
        /// Test link stealing where two links have the same link name but different link types. They should both be able to open without interfering each other.
        /// </summary>
        [Fact]
        public async Task LinkStealingDifferentLinkTypesTest()
        {
            await LinkStealingTestCase(sameType: false, closeLink1BeforeOpenLink2: false, linkRecoveryEnabled: false);
        }

        /// <summary>
        /// Test link stealing where two links have the same link name and type, but link1 is closed before link2 is opened. This should not trigger any link stealing at all.
        /// </summary>
        [Fact]
        public async Task LinkStealingCloseLink1TypesTest()
        {
            await LinkStealingTestCase(sameType: true, closeLink1BeforeOpenLink2: true, linkRecoveryEnabled: false);
        }

        /// <summary>
        /// Test link stealing where two links have the same link name and type. This should trigger link stealing and close link1 due to link stealing.
        /// </summary>
        [Fact]
        public async Task LinkStealingTest()
        {
            await LinkStealingTestCase(sameType: true, closeLink1BeforeOpenLink2: false, linkRecoveryEnabled: false);
        }

        /// <summary>
        /// Test link stealing where two links have the same link name but different link types with link recovery enabled.
        /// With link recovery enabled, link stealing would occur at the provider scope instead of the session scope.
        /// They should both be able to open without interfering each other.
        /// </summary>
        [Fact]
        public async Task LinkTerminusStealingDifferentLinkTypesTest()
        {
            await LinkStealingTestCase(sameType: false, closeLink1BeforeOpenLink2: false, linkRecoveryEnabled: true);
        }

        /// <summary>
        /// Test link stealing where two links have the same link name and type with link recovery enabled, but link1 is closed before link2 is opened.
        /// With link recovery enabled, link stealing would occur at the provider scope instead of the session scope.
        /// This should not trigger any link stealing at all.
        /// </summary>
        [Fact]
        public async Task LinkTerminusStealingCloseLink1TypesTest()
        {
            await LinkStealingTestCase(sameType: true, closeLink1BeforeOpenLink2: true, linkRecoveryEnabled: true);
        }

        /// <summary>
        /// Test link stealing where two links have the same link name and type with link recovery enabled.
        /// With link recovery enabled, link stealing would occur at the provider scope instead of the session scope.
        /// This should trigger link stealing and close link1 due to link stealing.
        /// </summary>
        [Fact]
        public async Task LinkTerminusStealingTest()
        {
            await LinkStealingTestCase(sameType: true, closeLink1BeforeOpenLink2: false, linkRecoveryEnabled: true);
        }

        /// <summary>
        /// Abort the link locally, which does not send a Detach to remote session, so the remote (broker) session will still have record of this link opened.
        /// Link stealing should happen at remote session, and remote session should discard its existing link record and allow the new link to be attached.
        /// </summary>
        [Fact]
        public async Task RemoteLinkStealingTest()
        {
            await LinkStealingTestCase(sameType: true, closeLink1BeforeOpenLink2: false, linkRecoveryEnabled: false, abortLink1: true);
        }

        /// <summary>
        /// Abort the link locally, which does not send a Detach to remote session, so the remote (broker) terminus will still have record of this link opened.
        /// Link stealing should happen at remote terminus, and remote terminus should discard its existing link record and allow the new link to be attached.
        /// </summary>
        [Fact]
        public async Task RemoteLinkTerminusStealingTest()
        {
            await LinkStealingTestCase(sameType: true, closeLink1BeforeOpenLink2: false, linkRecoveryEnabled: true, abortLink1: true);
        }

        /// <summary>
        /// Test case for link stealing scenarios, where two links will be opened sequentially and the first link will be checked if it was stolen or not by the second one.
        /// </summary>
        /// <param name="sameType">True if the two links opened will be of the same type. Different link types will avoid link stealing.</param>
        /// <param name="closeLink1BeforeOpenLink2">
        /// True if the first link should be closed before opening the second link. 
        /// If the first link is already closed, it should not have any impact on the opening of the second link, and link stealing would not occur.
        /// </param>
        /// <param name="linkRecoveryEnabled">
        /// True if link recovery is enabled. 
        /// This will involve uniqueness of link terminus objects, therefore link stealing will occur at the provider level, instead of just the session level.
        /// </param>
        /// <param name="abortLink1">
        /// True if link1 should be aborted before opening link2. Aborting the link will avoid sending a Detach frame to remote.
        /// This will cause the remote still have an instance of the link1 opened, it should be stolen and allow link2 with the same identifiers to open.
        /// </param>
        async Task LinkStealingTestCase(bool sameType, bool closeLink1BeforeOpenLink2, bool linkRecoveryEnabled, bool abortLink1 = false)
        {
            enableLinkRecovery = linkRecoveryEnabled;
            string linkName = nameof(LinkStealingTestCase) + Guid.NewGuid().ToString().Substring(0, 6);
            string queueName = "link-stealing-test-queue";
            AmqpConnection connection;
            if (linkRecoveryEnabled)
            {
                AmqpSettings settings = AmqpConnection.Factory.GetAmqpSettings(null);
                settings.RuntimeProvider = new TestRuntimeProvider();
                settings.TerminusStore = new AmqpInMemoryTerminusStore();
                TransportBase transport = await AmqpConnection.Factory.GetTransportAsync(connectionAddressUri, settings, AmqpConstants.DefaultTimeout, CancellationToken.None);
                connection = new AmqpConnection(transport, settings, new AmqpConnectionSettings() { ContainerId = Guid.NewGuid().ToString(), HostName = connectionAddressUri.Host });
                await connection.OpenAsync();
            }
            else
            {
                connection = await AmqpConnection.Factory.OpenConnectionAsync(connectionAddressUri, TimeSpan.FromSeconds(20));
            }

            AmqpSession link1Session = await connection.OpenSessionAsync();
            AmqpSession link2Session = linkRecoveryEnabled ? await connection.OpenSessionAsync() : link1Session;

            SendingAmqpLink link1 = await link1Session.OpenLinkAsync<SendingAmqpLink>(linkName, queueName);

            // Waiting here for the Flow frame to be sent by the opening receiving link.
            // Otherwise it may create a error if the Flow frame is received when the link is closed.
            await Task.Delay(1000);
            if (closeLink1BeforeOpenLink2)
            {
                await link1.CloseAsync();
            }
            else if (abortLink1)
            {
                link1.Abort();
            }

            bool shouldLinkBeStolen = sameType && !closeLink1BeforeOpenLink2 && !abortLink1;
            AmqpLink link2;
            if (sameType)
            {
                link2 = await link2Session.OpenLinkAsync<SendingAmqpLink>(linkName, queueName);
            }
            else
            {
                link2 = await link2Session.OpenLinkAsync<ReceivingAmqpLink>(linkName, queueName);
            }

            Assert.True(link2.State == AmqpObjectState.Opened);
            if (shouldLinkBeStolen)
            {
                Assert.True(link1.IsStolen());
            }
            else if (!sameType)
            {
                Assert.True(link1.State == AmqpObjectState.Opened);
            }

            await connection.CloseAsync();
        }
    }
}
