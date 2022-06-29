// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// This object is used to manage the link terminus objects across a container.
    /// It should be responsible for things like managing uniqueness and life cycle of link terminus objects, negotiating unsettled deliveries between local and remote link endpoints and managing link expiration policies.
    /// </summary>
    public class AmqpLinkTerminusManager : IAmqpLinkTerminusManager
    {
        object linkTerminiLock;
        IDictionary<AmqpLinkIdentifier, AmqpLinkTerminus> linkTermini;

        /// <summary>
        /// Create a new instance of <see cref="AmqpLinkTerminusManager"/>.
        /// </summary>
        public AmqpLinkTerminusManager()
        {
            this.linkTerminiLock = new object();
            this.linkTermini = new Dictionary<AmqpLinkIdentifier, AmqpLinkTerminus>();
        }

        /// <summary>
        /// Check if the given <see cref="AmqpLinkSettings"/> is for a recoverable link, or if a new recoverable link instance should be created out of the given settings.
        /// </summary>
        /// <param name="linkSettings">The link settings to be checked.</param>
        public static bool IsRecoverableLink(AmqpLinkSettings linkSettings)
        {
            AmqpSymbol expiryPolicy = linkSettings.GetExpiryPolicy();
            return IsValidTerminusExpirationPolicy(expiryPolicy);
        }

        /// <summary>
        /// Checks if the given <see cref="AmqpSymbol"/> is a valid link terminus expiration policy or not.
        /// </summary>
        public static bool IsValidTerminusExpirationPolicy(AmqpSymbol symbol)
        {
            return symbol.Equals(TerminusExpiryPolicy.LinkDetach) ||
                symbol.Equals(TerminusExpiryPolicy.SessionEnd) ||
                symbol.Equals(TerminusExpiryPolicy.ConnectionClose) ||
                symbol.Equals(TerminusExpiryPolicy.Never);
        }

        /// <summary>
        /// Try to get the link terminus with the given link identifier. Return true if the link terminus has been found.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object associated with the link identifier.</param>
        /// <returns>Returns true if there is a link terminus object found that was associated with the link identifier.</returns>
        public bool TryGetLinkTerminus(AmqpLinkIdentifier linkIdentifier, out AmqpLinkTerminus linkTerminus)
        {
            lock (this.linkTerminiLock)
            {
                return this.linkTermini.TryGetValue(linkIdentifier, out linkTerminus);
            }
        }

        /// <summary>
        /// Try to add a link terminus object which will be associated with the link identifier if there is no existing link terminus associated to the given link identifier.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object to be added.</param>
        /// <returns>Returns true if it was successfully added, or false if there is already an existing entry and the value is not added.</returns>
        public bool TryAddLinkTerminus(AmqpLinkIdentifier linkIdentifier, AmqpLinkTerminus linkTerminus)
        {
            lock (this.linkTerminiLock)
            {
                if (this.linkTermini.ContainsKey(linkIdentifier))
                {
                    return false;
                }

                this.linkTermini.Add(linkIdentifier, linkTerminus);
                return true;
            }
        }

        /// <summary>
        /// Remove the link terminus object that's identified by the link identifier if there is any.
        /// </summary>
        /// <param name="linkIdentifier">The link identifier used to get the associated link terminus object to be removed.</param>
        /// <param name="linkTerminus">The link terminus object associated with the given link identifier that's removed.</param>
        /// <returns>True if a link terminus object was identified and removed.</returns>
        public bool TryRemoveLinkTerminus(AmqpLinkIdentifier linkIdentifier, out AmqpLinkTerminus linkTerminus)
        {
            lock (this.linkTerminiLock)
            {
                if (this.linkTermini.TryGetValue(linkIdentifier, out linkTerminus))
                {
                    this.linkTermini.Remove(linkIdentifier);
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Remove the link terminus object that's identified by the given link identifier if there is any entry that matches both the key and value.
        /// This should be similar to ConcurrentDictionary.TryRemove(TKey, out TValue).
        /// </summary>
        /// <param name="item">The key value pair containing the link terminus to be removed and its identifier.</param>
        /// <returns>True if the key and value represented by item are successfully found and removed, false otherwise</returns>
        public bool TryRemoveLinkTerminus(KeyValuePair<AmqpLinkIdentifier, AmqpLinkTerminus> item)
        {
            bool removed = false;
            lock (this.linkTerminiLock)
            {
                if (this.linkTermini.TryGetValue(item.Key, out AmqpLinkTerminus linkTerminus) && linkTerminus == item.Value)
                {
                    this.linkTermini.Remove(item.Key);
                    removed = true;
                }
            }

            return removed;
        }

        /// <summary>
        /// Create a new <see cref="AmqpLinkTerminus"/> instance with the given link identifier and delivery store.
        /// </summary>
        /// <param name="linkIdentifier">The link identifier used by the new link terminus.</param>
        /// <param name="deliveryStore">The delivery store used by the new link terminus to save and retrieve deliveries.</param>
        /// <returns>A newly created instance of link terminus.</returns>
        public AmqpLinkTerminus CreateLinkTerminus(AmqpLinkIdentifier linkIdentifier, IAmqpDeliveryStore deliveryStore)
        {
            return new AmqpLinkTerminus(linkIdentifier, deliveryStore);
        }
    }
}
