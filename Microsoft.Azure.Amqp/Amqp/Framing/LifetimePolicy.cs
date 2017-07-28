// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class LifeTimePolicy : DescribedList
    {
        const int Fields = 0;

        protected LifeTimePolicy(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
        }

        protected override void OnDecode(ByteBuffer buffer, int count)
        {
        }

        protected override int OnValueSize()
        {
            return 0;
        }
    }
}
