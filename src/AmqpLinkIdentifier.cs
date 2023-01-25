// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// An object used to uniquely identify a link endpoint.
    /// </summary>
    public class AmqpLinkIdentifier
    {
        /// <summary>
        /// Construct an object used to uniquely identify a link endpoint by using the link name, the link's role (receiver/sender), and the containerId.
        /// </summary>
        public AmqpLinkIdentifier(string linkName, bool isReceiver, string containerId)
        {
            if (linkName == null)
            {
                throw new ArgumentNullException(nameof(linkName));
            }

            if (containerId == null)
            {
                throw new ArgumentNullException(nameof(containerId));
            }

            this.LinkName = linkName;
            this.IsReceiver = isReceiver;
            this.ContainerId = containerId;
        }

        /// <summary>
        /// Returns the link name.
        /// </summary>
        public string LinkName { get; }

        /// <summary>
        /// Returns the link role. True if this is used for a receiver, false if it's a sender.
        /// </summary>
        public bool IsReceiver { get; }

        /// <summary>
        /// Returns the containerId for the link endpoint.
        /// </summary>
        public string ContainerId { get; }

        /// <summary>
        /// Determines whether two link identifiers are equal based on <see cref="Attach.LinkName"/>
        /// and <see cref="Attach.Role"/>. Name comparison is case insensitive.
        /// </summary>
        /// <param name="obj">The object to compare with the current object.</param>
        /// <returns>True if the specified object is equal to the current object; otherwise, false.</returns>
        public override bool Equals(object obj)
        {
            AmqpLinkIdentifier other = obj as AmqpLinkIdentifier;
            if (other == null)
            {
                return false;
            }

            return this.LinkName.Equals(other.LinkName, StringComparison.OrdinalIgnoreCase)
                && this.IsReceiver == other.IsReceiver
                && this.ContainerId.Equals(other.ContainerId, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Gets a hash code of the object.
        /// </summary>
        public override int GetHashCode()
        {
            int hash = 29;
            hash = hash * 397 + StringComparer.OrdinalIgnoreCase.GetHashCode(this.LinkName);
            hash = hash * 397 + this.IsReceiver.GetHashCode();
            hash = hash * 397 + StringComparer.OrdinalIgnoreCase.GetHashCode(this.ContainerId);
            return hash;
        }

        /// <summary>
        /// Return the string representation of the link identifier.
        /// </summary>
        public override string ToString()
        {
            return $"{(this.IsReceiver ? "Receiver" : "Sender")}|LinkName={this.LinkName}|ContainerId={this.ContainerId}";
        }
    }
}
