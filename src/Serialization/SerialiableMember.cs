// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    /// <summary>
    /// Represents a serializable member.
    /// </summary>
    public sealed class SerialiableMember
    {
        /// <summary>Gets or sets the member name.</summary>
        public string Name
        {
            get;
            set;
        }

        /// <summary>Gets or sets the member order in the serialized form.</summary>
        public int Order
        {
            get;
            set;
        }

        /// <summary>Gets or sets a value indicating whether the member is mandatory.</summary>
        public bool Mandatory
        {
            get;
            set;
        }

        /// <summary>Gets or sets the member accessor.</summary>
        public MemberAccessor Accessor
        {
            get;
            set;
        }

        /// <summary>Gets or sets the serializable type of the member.</summary>
        public SerializableType Type
        {
            get;
            set;
        }
    }
}
