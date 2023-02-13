// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the message-annotations section of a message.
    /// </summary>
    public sealed class MessageAnnotations : DescribedAnnotations
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:message-annotations:map";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x0000000000000072;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public MessageAnnotations() : base(Name, Code)
        {
        }
    }
}
