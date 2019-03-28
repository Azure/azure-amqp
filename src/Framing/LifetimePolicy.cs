// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// The base class of life time policy of a dynamic node.
    /// </summary>
    public abstract class LifeTimePolicy : DescribedList
    {
        const int Fields = 0;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="name">The descriptor name.</param>
        /// <param name="code">The descriptor code.</param>
        protected LifeTimePolicy(AmqpSymbol name, ulong code)
            : base(name, code)
        {
        }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
        {
        }

        internal override int OnValueSize()
        {
            return 0;
        }
    }
}
