// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the delete-on-no-links-or-messages lifetime policy of a dynamic node.
    /// </summary>
    public sealed class DeleteOnNoLinksOrMessages : LifeTimePolicy
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:delete-on-no-links-or-messages:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x000000000000002e;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public DeleteOnNoLinksOrMessages() : base(Name, Code)
        {
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return "delete-on-no-links-or-messages()";
        }
    }
}
