namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using TestAmqpBroker;
    using Xunit;

    public class TestAmqpBrokerFixture : IDisposable
    {
        const string address = "amqp://localhost:15672";

        public TestAmqpBrokerFixture()
        {
            this.Address = new Uri(address);
            this.Broker = new TestAmqpBroker(new string[] { address }, "guest:guest", null, null);
            this.Broker.Start();
        }

        public Uri Address { get; }

        public TestAmqpBroker Broker { get; }

        public void Dispose()
        {
            this.Broker.Stop();
        }
    }
}