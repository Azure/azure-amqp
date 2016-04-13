// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public sealed class Footer : DescribedAnnotations
    {
        public static readonly string Name = "amqp:footer:map";
        public static readonly ulong Code = 0x0000000000000078;

        public Footer() : base(Name, Code)
        {
        }

        public override string ToString()
        {
            return "footer";
        }
    }
}
