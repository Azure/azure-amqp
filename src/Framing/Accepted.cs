// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the accepted outcome.
    /// </summary>
    public sealed class Accepted : Outcome
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public const string Name = "amqp:accepted:list";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public const ulong Code = 0x0000000000000024;
        const int Fields = 0;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Accepted() : base(Name, Code)
        {
        }

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
            return "accepted()";
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
