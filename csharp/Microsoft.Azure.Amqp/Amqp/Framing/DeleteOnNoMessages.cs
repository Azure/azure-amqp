// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    sealed class DeleteOnNoMessages : LifeTimePolicy
    {
        public static readonly string Name = "amqp:delete-on-no-messages:list";
        public static readonly ulong Code = 0x000000000000002d;

        public DeleteOnNoMessages() : base(Name, Code)
        {
        }

        public override string ToString()
        {
            return "delete-on-no-messages()";
        }
    }
}
