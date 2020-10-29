// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using global::Microsoft.Azure.Amqp;
    using System;
    using System.Collections.Generic;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpExtensionsTests
    {
        [Fact]
        public void TestFind()
        {
            Dictionary<Type, object> dictionary = new Dictionary<Type, object>();

            Assert.Null(dictionary.Find<TestClass>());
            
            var testValue = new TestClass();
            dictionary.Add(typeof(TestClass), testValue);
            Assert.Same(testValue, dictionary.Find<TestClass>());

            dictionary.Remove(typeof(TestClass));

            Assert.Null(dictionary.Find<TestClass>());
        }

        private class TestClass
        {
        }
    }
}
