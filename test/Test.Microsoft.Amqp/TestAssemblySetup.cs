namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    class TestAssemblySetup
    {
        [AssemblyInitialize]
        public static void Initialize(TestContext context)
        {
            //Environment.SetEnvironmentVariable("AMQP_DEBUG", "1");
        }

        [AssemblyCleanup()]
        public static void AssemblyCleanup()
        {
        }
    }
}
