// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Collections;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class AmqpSequence : DescribedList
    {
        public static readonly string Name = "amqp:amqp-sequence:list";
        public static readonly ulong Code = 0x0000000000000076;

        IList innerList;

        public AmqpSequence()
            : this(new List<object>()) 
        {
        }

        public AmqpSequence(IList innerList)
            : base(Name, Code)
        {
            this.innerList = innerList;
        }

        public IList List
        {
            get { return this.innerList; }
        }

        protected override int FieldCount
        {
            get { return this.innerList.Count; }
        }

        public override string ToString()
        {
            return "sequence()";
        }

        protected override int OnValueSize()
        {
            return ListEncoding.GetValueSize(this.innerList);
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
            foreach (object item in this.innerList)
            {
                AmqpEncoding.EncodeObject(item, buffer);
            }
        }

        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            for (int i = 0; i < count; i++)
            {
                this.innerList.Add(AmqpEncoding.DecodeObject(buffer));
            }
        }
    }
}
