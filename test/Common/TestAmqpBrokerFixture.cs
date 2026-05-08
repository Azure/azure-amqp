namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;
    using TestAmqpBroker;

    [TestClass]
    public class TestAmqpBrokerFixture
    {
        const string address = "amqp://localhost:15672";
        const string wsAddress = "ws://localhost:15678";
        static TestAmqpBroker broker;

        public static Uri Address = new Uri(address);

        public static Uri WsAddress = new Uri(wsAddress);

        public static TestAmqpBroker Broker => broker;

        [AssemblyInitialize]
        public static void AssemblyInitialize(TestContext context)
        {
            broker = new TestAmqpBroker(new string[] { address, wsAddress }, "guest:guest", null, null);
            broker.Start();
        }

        [AssemblyCleanup]
        public static void AssemblyCleanup()
        {
            broker?.Stop();
            broker = null;
        }
    }
}
