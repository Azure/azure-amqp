namespace Test.Microsoft.Azure.Amqp
{
    static class TestCategory
    {
#if NETSTANDARD
        public const string Current = ".Net Core";
#else
        public const string Current = "Full .NET";
#endif
    }
}
