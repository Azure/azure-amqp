// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AmqpExtensionsTests
    {
        [TestMethod]
        public void TryAddRefOnlyAcquiresLiveReferences()
        {
            int refs = 1;
            Assert.IsTrue(Extensions.TryAddRef(ref refs));
            Assert.AreEqual(2, refs);

            refs = 0;
            Assert.IsFalse(Extensions.TryAddRef(ref refs));
            Assert.AreEqual(0, refs);

            refs = -1;
            Assert.IsFalse(Extensions.TryAddRef(ref refs));
            Assert.AreEqual(-1, refs);

            refs = int.MaxValue;
            Assert.ThrowsException<OverflowException>(() => Extensions.TryAddRef(ref refs));
            Assert.AreEqual(int.MaxValue, refs);
        }

        [TestMethod]
        public void TryAddRefPreservesConcurrentOwnership()
        {
            int refs = 1;
            Parallel.For(0, 10000, _ =>
            {
                Assert.IsTrue(Extensions.TryAddRef(ref refs));
            });
            Assert.AreEqual(10001, refs);
            Parallel.For(0, 10000, _ => Interlocked.Decrement(ref refs));
            Assert.AreEqual(1, refs);
            Assert.AreEqual(0, Interlocked.Decrement(ref refs));

            Parallel.For(0, 10000, _ =>
            {
                Assert.IsFalse(Extensions.TryAddRef(ref refs));
            });
            Assert.AreEqual(0, refs);
        }

        [TestMethod]
        public void ByteBufferSliceKeepsParentAlive()
        {
            var buffer = new ByteBuffer(16, false);
            buffer.Append(16);
            byte[] array = buffer.Buffer;
            var slice = buffer.GetSlice(0, 8);
            Assert.IsTrue(slice.TryAddReference());
            buffer.Dispose();
            slice.Dispose();
            Assert.AreSame(array, buffer.Buffer);
            slice.Dispose();
            Assert.IsNull(buffer.Buffer);
            Assert.IsFalse(slice.TryAddReference());
            Assert.IsFalse(buffer.TryAddReference());
        }

        [TestMethod]
        public void TestFind()
        {
            Dictionary<Type, object> dictionary = new Dictionary<Type, object>();

            Assert.IsNull(dictionary.Find<TestClass>());
            
            var testValue = new TestClass();
            dictionary.Add(typeof(TestClass), testValue);
            Assert.AreSame(testValue, dictionary.Find<TestClass>());

            dictionary.Remove(typeof(TestClass));

            Assert.IsNull(dictionary.Find<TestClass>());
        }

        private class TestClass
        {
        }
    }
}
