// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    class TerminusExpiryPolicy
    {
        TerminusExpiryPolicy()
        {
        }

        public static readonly AmqpSymbol LinkDetach = "link-detach";
        public static readonly AmqpSymbol SessionEnd = "session-end";
        public static readonly AmqpSymbol ConnectionClose = "connection-close";
        public static readonly AmqpSymbol Never = "never";
    }
}
