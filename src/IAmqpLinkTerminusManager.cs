// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Framing;
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// This object is used to manage the link terminus objects across a container.
    /// It should be responsible for things like managing uniqueness and life cycle of link terminus objects, negotiating unsettled deliveries between local and remote link endpoints and managing link expiration policies.
    /// </summary>
    public interface IAmqpLinkTerminusManager
    {
        /// <summary>
        /// Try to get the link terminus with the given link identifier. Return true if the link terminus has been found.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object associated with the link identifier.</param>
        /// <returns>True if there is a link terminus object found that was associated with the link identifier.</returns>
        bool TryGetLinkTerminus(AmqpLinkIdentifier linkIdentifier, out AmqpLinkTerminus linkTerminus);

        /// <summary>
        /// Try to add a link terminus object which will be identified by the link identifier if there is no existing link terminus associated to the given link identifier.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object to be added.</param>
        /// <returns>True if it was successfully added, or false if there is already an existing entry and the value is not added.</returns>
        bool TryAddLinkTerminus(AmqpLinkIdentifier linkIdentifier, AmqpLinkTerminus linkTerminus);

        /// <summary>
        /// Remove the link terminus object that's identified by the given link identifier if there is any entry that matches both the key and value.
        /// This should be similar to ConcurrentDictionary.TryRemove(TKey, out TValue).
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used to identify the link terminus to be removed.</param>
        /// <param name="linkTerminus">The existing link terminus to be removed, if any.</param>
        /// <returns>True if the key and value represented by item are successfully found and removed, false otherwise.</returns>
        bool TryRemoveLinkTerminus(AmqpLinkIdentifier linkIdentifier, out AmqpLinkTerminus linkTerminus);

        /// <summary>
        /// Remove the link terminus object that's identified by the given link identifier if there is any entry that matches both the key and value.
        /// This should be similar to ConcurrentDictionary.TryRemove(TKey, out TValue).
        /// </summary>
        /// <param name="item">The key value pair containing the link terminus to be removed and its identifier.</param>
        /// <returns>True if the key and value represented by item are successfully found and removed, false otherwise.</returns>
        bool TryRemoveLinkTerminus(KeyValuePair<AmqpLinkIdentifier, AmqpLinkTerminus> item);

        /// <summary>
        /// Negotiate and consolidate the unsettled deliveries map from the remote Attach and the local link terminus associated with the given link identifier.
        /// </summary>
        /// <param name="linkIdentifier">The link identifier used to find the associated local link terminus.</param>
        /// <param name="remoteAttach">The <see cref="Attach"/> received from remote as part of a link open process.</param>
        /// <returns>
        /// A task containing a map of deliveries which should remain to be unsettled for the local link terminus.
        /// For senders, this means that these unsettled deliveries may need to be redelivered to remote.
        /// For receivers, this means that these unsettled deliveries have already been previously delivered, so they may be skipped or directly settled in case of redelivery from remote sender.
        /// </returns>
        Task<IDictionary<ArraySegment<byte>, Delivery>> ProcessRemoteUnsettledMapAsync(AmqpLinkIdentifier linkIdentifier, Attach remoteAttach);

        /// <summary>
        /// Create a new <see cref="AmqpLinkTerminus"/> instance with the given link identifier and delivery store.
        /// </summary>
        /// <param name="linkIdentifier">The link identifier used by the new link terminus.</param>
        /// <param name="deliveryStore">The delivery store used by the new link terminus to save and retrieve deliveries.</param>
        /// <returns>A newly created instance of link terminus.</returns>
        AmqpLinkTerminus CreateLinkTerminus(AmqpLinkIdentifier linkIdentifier, IAmqpDeliveryStore deliveryStore);
    }
}
