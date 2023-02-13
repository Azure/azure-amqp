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

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Accepted() : base(Name, Code, 0)
        {
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return "accepted()";
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
        }

        /// <summary>
        /// Decodes the fields from the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        /// <param name="count">The number of fields.</param>
        protected override void OnDecode(ByteBuffer buffer, int count)
        {
        }

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
        protected override int OnValueSize()
        {
            return 0;
        }
    }
}
