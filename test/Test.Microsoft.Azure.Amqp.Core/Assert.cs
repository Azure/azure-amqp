namespace Test.Microsoft.Azure.Amqp
{
    static class Assert
    {
        public static void IsTrue(bool b, string m = null)
        {
            Xunit.Assert.True(b, m);
        }

        public static void IsFalse(bool b, string m = null)
        {
            Xunit.Assert.False(b, m);
        }

        public static void IsNull(object o, string m = null)
        {
            Xunit.Assert.True(o == null, m);
        }

        public static void IsNotNull(object o, string m = null)
        {
            Xunit.Assert.True(o != null, m);
        }

        public static void AreEqual(object a, object b, string m = null)
        {
            Xunit.Assert.Equal(a, b);
        }
    }
}
