// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    public sealed class Released : Outcome
    {
        public static readonly string Name = "amqp:released:list";
        public static readonly ulong Code = 0x0000000000000026;
        const int Fields = 0;

        public Released() : base(Name, Code) { }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        public override string ToString()
        {
            return "released()";
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
