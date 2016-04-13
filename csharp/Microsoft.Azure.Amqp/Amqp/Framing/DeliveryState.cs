// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class DeliveryState : DescribedList
    {
        public DeliveryState(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }
    }
}
