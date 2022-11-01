// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Transport;
    using TestAmqpBroker;
    using Xunit;

    [CollectionDefinition(nameof(SequentialTests), DisableParallelization = true)]
    public class SequentialTests { }

    [Collection(nameof(SequentialTests))]
    [Trait("Category", TestCategory.Current)]
    public class AmqpConnectionTests
    {
        const string address = "amqp://localhost:15672";

        [Fact]
        public void AmqpConnectionNullContainerIdTest()
        {
            var broker = new TestAmqpBroker(new string[] { address }, "guest:guest", null, null);
            broker.Start();

            try
            {
                var uri = new Uri(address);
                TransportBase transport = AmqpUtils.CreateTransport(uri.Host, uri.Port, null, false, null);
                AmqpSettings settings = AmqpUtils.GetAmqpSettings(true, null, false);
                var connectionSettings = new AmqpConnectionSettings();
                var connection = new AmqpConnection(transport, settings, connectionSettings);
                Assert.Throws<ArgumentNullException>(() => connection.Open());
                transport.Close();
            }
            finally
            {
                broker.Stop();
            }
        }

        [Fact]
        public void AmqpConcurrentConnectionsTest()
        {
            var broker = new TestAmqpBroker(new string[] { address }, "guest:guest", null, null);
            broker.Start();

            Exception lastException = null;
            Action action = () =>
            {
                try
                {
                    AmqpConnection connection = AmqpUtils.CreateConnection(
                        new Uri(address),
                        null,
                        false,
                        null,
                        (int)AmqpConstants.DefaultMaxFrameSize);
                    connection.Open();
                    connection.Close();
                }
                catch (Exception exp)
                {
                    lastException = exp;
                }
            };

            Task[] tasks = new Task[32];
            for (int i = 0; i < tasks.Length; ++i)
            {
                tasks[i] = Task.Run(action);
            }

            Task.WaitAll(tasks);

            broker.Stop();

            Assert.True(lastException == null, string.Format("Failed. Last exception {0}", lastException == null ? string.Empty : lastException.ToString()));
        }
    }
}
