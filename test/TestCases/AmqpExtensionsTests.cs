// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AmqpExtensionsTests
    {
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
