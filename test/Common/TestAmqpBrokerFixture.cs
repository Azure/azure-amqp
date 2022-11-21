namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using global::Microsoft.Azure.Amqp;
    using TestAmqpBroker;

    public class TestAmqpBrokerFixture : IDisposable
    {
        const string address = "amqp://localhost:15672";

        public static Uri Address = new Uri(address);

        public TestAmqpBrokerFixture()
        {
            // AmqpTrace.FrameLogger = s => System.Diagnostics.Trace.WriteLine(s);
            this.Broker = new TestAmqpBroker(new string[] { address }, "guest:guest", null, null);

#if !WINDOWS_UWP
            if (Process.GetProcessesByName("TestAmqpBroker").Length == 0)
            {
                this.Broker.Start();
            }
#endif
        }

        public TestAmqpBroker Broker { get; }

        public void Dispose()
        {
            this.Broker.Stop();
        }
    }
}