// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    class DistributionMode
    {
        public static readonly AmqpSymbol Move = "move";
        public static readonly AmqpSymbol Copy = "copy";

        DistributionMode()
        {
        }
    }
}