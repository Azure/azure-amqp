namespace Test.Microsoft.Azure.Amqp
{
    static class TestCategory
    {
#if DOTNET_CORE
        public const string Current = ".Net Core";
#else
        public const string Current = "Full .NET";
#endif
    }
}
