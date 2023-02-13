// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the footer section of a message.
    /// </summary>
    public sealed class Footer : DescribedAnnotations
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:footer:map";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000078;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Footer() : base(Name, Code)
        {
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return "footer";
        }
    }
}
