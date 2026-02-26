// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the delivery-annotations section of a message.
    /// </summary>
    public sealed class DeliveryAnnotations : DescribedAnnotations
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:delivery-annotations:map";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000071;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public DeliveryAnnotations() : base(Name, Code)
        {
        }
    }
}
