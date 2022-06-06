// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Framing;
    using System;

    /// <summary>
    /// An object used to uniquely identify a link endpoint.
    /// </summary>
    public class AmqpLinkIdentifier
    {
        string linkName;
        bool? role;

        /// <summary>
        /// Constructor an object used to uniquely identify a link endpoint by using the link name and the link's role (sender/receiver).
        /// </summary>
        public AmqpLinkIdentifier(string linkName, bool? role)
        {
            this.linkName = linkName;
            this.role = role;
        }

        /// <summary>
        /// Returns the link name.
        /// </summary>
        public string Name { get => this.linkName; }

        /// <summary>
        /// Returns the link role. True if this is used for a receiver, false if it's a sender.
        /// </summary>
        public bool? Role { get => this.role; }

        /// <summary>
        /// Determines whether two link identifiers are equal based on <see cref="Attach.LinkName"/>
        /// and <see cref="Attach.Role"/>. Name comparison is case insensitive.
        /// </summary>
        /// <param name="obj">The object to compare with the current object.</param>
        /// <returns>True if the specified object is equal to the current object; otherwise, false.</returns>
        public override bool Equals(object obj)
        {
            AmqpLinkIdentifier other = obj as AmqpLinkIdentifier;
            if (other == null || other.linkName == null)
            {
                return false;
            }

            return this.linkName.Equals(other.linkName, StringComparison.CurrentCultureIgnoreCase) && this.role == other.role;
        }

        /// <summary>
        /// Gets a hash code of the object.
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (this.linkName.GetHashCode() * 397) + this.role.GetHashCode();
        }
    }
}
