namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ListAdapterTests
    {
        [TestMethod]
        public void DetachedStateIsInvalid()
        {
            var adapter = new ListAdapter<int, string>(value => value.ToString());
            adapter.Attach(new List<int> { 1 });
            adapter.Detach();

            Assert.ThrowsException<InvalidOperationException>(() => adapter.Count);
            Assert.ThrowsException<InvalidOperationException>(() => adapter.Contains("1"));
            Assert.ThrowsException<InvalidOperationException>(() => adapter.CopyTo(new string[1], 0));
            Assert.ThrowsException<InvalidOperationException>(() => adapter.GetEnumerator());
        }

        [TestMethod]
        public void AttachTwiceIsInvalid()
        {
            var adapter = new ListAdapter<int, string>(value => value.ToString());
            adapter.Attach(new List<int> { 1 });

            Assert.ThrowsException<InvalidOperationException>(() => adapter.Attach(new List<int> { 2 }));
        }

        [TestMethod]
        public void DetachIsIdempotent()
        {
            var adapter = new ListAdapter<int, string>(value => value.ToString());

            adapter.Detach();
            adapter.Attach(new List<int> { 1 });
            adapter.Detach();
            adapter.Detach();
        }
    }
}
