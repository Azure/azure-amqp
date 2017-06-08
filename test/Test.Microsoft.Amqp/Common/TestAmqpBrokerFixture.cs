namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using TestAmqpBroker;

    public class TestAmqpBrokerFixture : IDisposable
    {
        const string address = "amqp://localhost:15672";
        bool started;

        public TestAmqpBrokerFixture()
        {
            this.Address = new Uri(address);
            this.Broker = new TestAmqpBroker(new string[] { address }, "guest:guest", null, null);

            if (Process.GetProcessesByName("TestAmqpBroker").Length == 0)
            {
                this.Broker.Start();
                this.started = true;
            }
        }

        public Uri Address { get; }

        public TestAmqpBroker Broker { get; }

        public void Dispose()
        {
            if (this.started)
            {
                this.Broker.Stop();
            }
        }
    }
}