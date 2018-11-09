// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public sealed class DeliveryAnnotations : DescribedAnnotations
    {
        public static readonly string Name = "amqp:delivery-annotations:map";
        public static readonly ulong Code = 0x0000000000000071;

        public DeliveryAnnotations() : base(Name, Code)
        {
        }
    }
}
