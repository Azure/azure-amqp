// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the rejected outcome.
    /// </summary>
    public sealed class Released : Outcome
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:released:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000026;
        const int Fields = 0;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Released() : base(Name, Code) { }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
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
