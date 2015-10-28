// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class LinkPerformative : Performative
    {
        protected LinkPerformative(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }

        public uint? Handle { get; set; }
    }
}
