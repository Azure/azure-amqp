// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System.Collections.Generic;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;
    using global::Microsoft.Azure.Amqp;

    [TestClass]
    public class AmqpLinkIdentifierTest
    {
        [TestMethod]
        public void LinkIdentifierTest()
        {
            var original = new AmqpLinkIdentifier("Sender", false, "ContainerID");
            IDictionary<AmqpLinkIdentifier, object> dictionary = new Dictionary<AmqpLinkIdentifier, object>();
            dictionary.Add(original, new object());

            // link name is case insensitive
            Assert.IsTrue(dictionary.ContainsKey(new AmqpLinkIdentifier("sender", false, "ContainerID")));
            Assert.AreEqual(original, new AmqpLinkIdentifier("sender", false, "ContainerID"));

            // containerId is case insensitive
            Assert.IsTrue(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender", false, "containerid")));
            Assert.AreEqual(original, new AmqpLinkIdentifier("Sender", false, "containerid"));

            // different linkNames
            Assert.IsFalse(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender1", false, "ContainerID")));
            Assert.AreNotEqual(original, new AmqpLinkIdentifier("Sender1", false, "ContainerID"));

            // different roles
            Assert.IsFalse(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender", true, "ContainerID")));
            Assert.AreNotEqual(original, new AmqpLinkIdentifier("Sender", true, "ContainerID"));

            // different containerId
            Assert.IsFalse(dictionary.ContainsKey(new AmqpLinkIdentifier("Sender", false, "ContainerID1")));
            Assert.AreNotEqual(original, new AmqpLinkIdentifier("Sender", false, "ContainerID1"));
        }
    }
}
