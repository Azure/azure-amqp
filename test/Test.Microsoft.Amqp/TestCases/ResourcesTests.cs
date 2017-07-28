// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Reflection;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class ResourcesTests
    {
        [Fact]
        public void AmqpResourcesTest()
        {
            TestResource(Type.GetType("Microsoft.Azure.Amqp.Amqp.Resources, Microsoft.Azure.Amqp"));
            TestResource(Type.GetType("Microsoft.Azure.Amqp.ResourcesGeneric, Microsoft.Azure.Amqp"));
        }

        static void TestResource(Type type)
        {
            int count = 0;
            PropertyInfo[] properties = type.GetProperties(BindingFlags.Static | BindingFlags.NonPublic);
            foreach (PropertyInfo pi in properties)
            {
                if (pi.PropertyType == typeof(string))
                {
                    object value = pi.GetValue(null);
                    System.Diagnostics.Debug.WriteLine("{0}={1}", pi.Name, value);
                    count++;
                    Assert.NotNull(value);
                    Assert.True(value is string);
                }
            }
            Assert.True(count > 0, "didn't find any resource strings.");
        }
    }
}
