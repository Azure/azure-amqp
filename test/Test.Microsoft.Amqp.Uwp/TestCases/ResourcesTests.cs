// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Reflection;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Amqp;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class ResourcesTests
    {
        [Fact]
        public void AmqpResourcesTest()
        {
            TestResource(typeof(Resources));
            TestResource(typeof(ResourcesGeneric));
        }

        static void TestResource(Type type)
        {
            PropertyInfo[] properties = type.GetProperties(BindingFlags.Static | BindingFlags.NonPublic);
            foreach (PropertyInfo pi in properties)
            {
                if (pi.PropertyType == typeof(string))
                {
                    object value = pi.GetValue(null);
                    System.Diagnostics.Debug.WriteLine("{0}={1}", pi.Name, value);
                    Assert.NotNull(value);
                    Assert.True(value is string);
                }
            }
        }
    }
}
