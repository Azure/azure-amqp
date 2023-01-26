// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System.Collections.Generic;
    using Xunit;
    using global::Microsoft.Azure.Amqp;

    [Trait("Category", TestCategory.Current)]
    public class AmqpLinkIdentifierTest
    {
        [Fact]
        public void LinkIdentifierTest()
        {
            var original = new AmqpLinkIdentifier("Sender", false, "ContainerID");
            IDictionary<AmqpLinkIdentifier, object> dictionary = new Dictionary<AmqpLinkIdentifier, object>();
            dictionary.Add(original, new object());

            // link name is case insensitive
            Assert.True(dictionary.ContainsKey(new AmqpLinkIdentifier("sender", false, "ContainerID")));
            Assert.Equal(original, new AmqpLinkIdentifier("sender", false, "ContainerID"));

            // containerId is case insensitive
            Assert.True(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender", false, "containerid")));
            Assert.Equal(original, new AmqpLinkIdentifier("Sender", false, "containerid"));

            // different linkNames
            Assert.False(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender1", false, "ContainerID")));
            Assert.NotEqual(original, new AmqpLinkIdentifier("Sender1", false, "ContainerID"));

            // different roles
            Assert.False(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender", true, "ContainerID")));
            Assert.NotEqual(original, new AmqpLinkIdentifier("Sender", true, "ContainerID"));

            // different containerId
            Assert.False(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender", false, "ContainerID1")));
            Assert.NotEqual(original, new AmqpLinkIdentifier("Sender", false, "ContainerID1"));
        }
    }
}
