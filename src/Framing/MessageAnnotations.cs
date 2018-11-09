// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public sealed class MessageAnnotations : DescribedAnnotations
    {
        public static readonly string Name = "amqp:message-annotations:map";
        public static readonly ulong Code = 0x0000000000000072;

        public MessageAnnotations() : base(Name, Code)
        {
        }
    }
}
