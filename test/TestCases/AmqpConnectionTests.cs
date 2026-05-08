// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;
    using TestAmqpBroker;

    [TestClass]
    public class AmqpConnectionTests
    {
        [TestMethod]
        public async Task AmqpsConnectionLoopTest()
        {
            const string address = "amqps://[::1]:15672";
            var broker = new TestAmqpBroker(new string[] { address }, null, "localhost", null);
            broker.Start();

            for (int i = 0; i < 500; ++i)
            {
                AmqpConnectionFactory factory = new AmqpConnectionFactory();
                factory.TlsSettings.CertificateValidationCallback = (a, b, c, d) => true;
                var connection = await factory.OpenConnectionAsync(new Uri(address), TimeSpan.FromSeconds(30));
                await connection.CloseAsync(TimeSpan.FromSeconds(30));
            }

            broker.Stop();
        }

        [TestMethod]
        public void AmqpConnectionNullContainerIdTest()
        {
            const string address = "amqp://localhost:15672";
            var broker = new TestAmqpBroker(new string[] { address }, "guest:guest", null, null);
            broker.Start();

            try
            {
                var uri = new Uri(address);
                TransportBase transport = AmqpUtils.CreateTransport(uri.Host, uri.Port, null, false, null);
                AmqpSettings settings = AmqpUtils.GetAmqpSettings(true, null, false);
                var connectionSettings = new AmqpConnectionSettings();
                var connection = new AmqpConnection(transport, settings, connectionSettings);
                Assert.ThrowsException<ArgumentNullException>(() => connection.Open());
                transport.Close();
            }
            finally
            {
                broker.Stop();
            }
        }

        [TestMethod]
        public void AmqpConcurrentConnectionsTest()
        {
            const string address = "amqp://localhost:15672";
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

            Assert.IsTrue(lastException == null, string.Format("Failed. Last exception {0}", lastException == null ? string.Empty : lastException.ToString()));
        }
    }
}
