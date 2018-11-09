// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public sealed class Accepted : Outcome
    {
        public static readonly string Name = "amqp:accepted:list";
        public static readonly ulong Code = 0x0000000000000024;
        const int Fields = 0;

        public Accepted() : base(Name, Code)
        {
        }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        public override string ToString()
        {
            return "accepted()";
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
