// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using Microsoft.Azure.Amqp.Encoding;
using Microsoft.Azure.Amqp.Framing;

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// A class which contains info and properties about a link terminus.
    /// </summary>
    public interface IAmqpLinkTerminusInfo
    {
        /// <summary>
        /// Gets or sets the "address" field.
        /// </summary>
        public Address Address { get; set; }

        /// <summary>
        /// Gets or sets the "durable" field.
        /// </summary>
        public uint? Durable { get; set; }

        /// <summary>
        /// Gets or sets the "expiry-policy" field.
        /// </summary>
        public AmqpSymbol ExpiryPolicy { get; set; }

        /// <summary>
        /// Gets or sets the "timeout" field.
        /// </summary>
        public uint? Timeout { get; set; }

        /// <summary>
        /// Gets or sets the "dynamic" field.
        /// </summary>
        public bool? Dynamic { get; set; }

        /// <summary>
        /// Gets or sets the "dynamic-node-properties" field.
        /// </summary>
        public Fields DynamicNodeProperties { get; set; }

        /// <summary>
        /// Gets or sets the "capabilities" field.
        /// </summary>
        public Multiple<AmqpSymbol> Capabilities { get; set; }
    }
}
