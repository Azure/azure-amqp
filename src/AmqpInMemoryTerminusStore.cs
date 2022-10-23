// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// This class is the in-memory implemenation of <see cref="IAmqpTerminusStore"/>
    /// Will be used to store and retrieve both linkTerminus Information and deliveries associated with the termini.
    /// </summary>
    public class AmqpInMemoryTerminusStore : IAmqpTerminusStore
    {
        readonly object thisLock;
        readonly IDictionary<AmqpLinkIdentifier, AmqpLinkTerminus> linkTermini;
        readonly IDictionary<AmqpLinkTerminus, IDictionary<ArraySegment<byte>, Delivery>> deliveries;

        /// <summary>
        /// Create an object that will be used to save and retrieve deliveries in memory.
        /// </summary>
        public AmqpInMemoryTerminusStore()
        {
            this.thisLock = new object();
            this.linkTermini = new Dictionary<AmqpLinkIdentifier, AmqpLinkTerminus>();
            this.deliveries = new Dictionary<AmqpLinkTerminus, IDictionary<ArraySegment<byte>, Delivery>>();
        }

        #region TerminusManagement

        /// <summary>
        /// Try to add a link terminus object which will be associated with the link identifier if there is no existing link terminus associated to the given link identifier.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object to be added.</param>
        /// <returns>Returns true if it was successfully added, or false if there is already an existing entry and the value is not added.</returns>
        public Task<bool> TryAddLinkTerminusAsync(AmqpLinkIdentifier linkIdentifier, AmqpLinkTerminus linkTerminus)
        {
            lock (this.thisLock)
            {
                if (this.linkTermini.ContainsKey(linkIdentifier))
                {
                    return Task.FromResult(false);
                }

                this.linkTermini.Add(linkIdentifier, linkTerminus);
                return Task.FromResult(true);
            }
        }

        /// <summary>
        /// Remove the link terminus object that's identified by the given link identifier if there is any entry that matches both the key and value.
        /// This will also remove the deliveries associated with the terminus.
        /// This should be similar to ConcurrentDictionary.TryRemove(TKey, out TValue).
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object to be removed.</param>
        /// <returns>True if the linkTerminus object was successfully found and removed, false otherwise.</returns>
        public Task<bool> TryRemoveLinkTerminusAsync(AmqpLinkIdentifier linkIdentifier, AmqpLinkTerminus linkTerminus)
        {
            bool removed = false;
            lock (this.thisLock)
            {
                if (this.linkTermini.TryGetValue(linkIdentifier, out AmqpLinkTerminus registeredLinkTerminus) && registeredLinkTerminus == linkTerminus)
                {
                    // Remove the deliveries associated with the terminus
                    if (this.deliveries.TryGetValue(linkTerminus, out IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries))
                    {
                        terminusDeliveries.Clear();
                        terminusDeliveries = null;
                    }

                    this.deliveries.Remove(linkTerminus);

                    // Remove the Terminus
                    this.linkTermini.Remove(linkIdentifier);
                    removed = true;
                }
            }

            return Task.FromResult(removed);
        }

        /// <summary>
        /// Try to get the link terminus with the given link identifier. Return true if the link terminus has been found.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object associated with the link identifier.</param>
        /// <returns>True if there is a link terminus object was found associated with the given link identifier.</returns>
        public Task<bool> TryGetLinkTerminusAsync(AmqpLinkIdentifier linkIdentifier, out AmqpLinkTerminus linkTerminus)
        {
            lock (this.thisLock)
            {
                bool foundTerminus = this.linkTermini.TryGetValue(linkIdentifier, out linkTerminus);

                // If found, also find the associated Unsettled deliveries for the Terminus
                if (foundTerminus)
                {
                    if (this.deliveries.TryGetValue(linkTerminus, out IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries))
                    {
                        linkTerminus.UnsettledDeliveries = new Dictionary<ArraySegment<byte>, Delivery>(terminusDeliveries); 
                    }
                }

                return Task.FromResult(foundTerminus);
            }
        }

        #endregion

        #region DeliveryManagement

        /// <summary>
        /// Save a collection of <see cref="Delivery"/> under the given link terminus to be retrieved later.
        /// </summary>
        /// <param name="linkTerminus">The link terminus associated with the deliveries.</param>
        /// <param name="unsettledDeliveries">A dictionary of Key-Value pairs of DeliveryTag and corresponding delivery.</param>
        /// <returns>True if the deliveries were successfully saved under the link terminus.</returns>
        public Task SaveDeliveriesAsync(AmqpLinkTerminus linkTerminus, IDictionary<ArraySegment<byte>, Delivery> unsettledDeliveries)
        {
            if (unsettledDeliveries == null || unsettledDeliveries.Count == 0)
            {
                return Task.CompletedTask;
            }

            lock (this.thisLock)
            {
                IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries;
                if (!this.deliveries.TryGetValue(linkTerminus, out terminusDeliveries))
                {
                    terminusDeliveries = new Dictionary<ArraySegment<byte>, Delivery>();
                    this.deliveries.Add(linkTerminus, terminusDeliveries);
                }

                foreach (var keyValuePair in unsettledDeliveries)
                {
                    terminusDeliveries.Remove(keyValuePair.Key);
                    terminusDeliveries.Add(keyValuePair.Key, keyValuePair.Value); 
                }
            }

            return Task.CompletedTask;
        }

        /// <summary>
        /// Try to remove a delivery that's stored under the given link terminus with the given delivery tag, if any.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the removed delivery belongs to.</param>
        /// <param name="deliveryTag">The delivery tag of the delivery to be removed.</param>
        /// <returns>True if the delivery was found and removed, false otherwise.</returns>
        public Task RemoveDeliveryAsync(AmqpLinkTerminus linkTerminus, ArraySegment<byte> deliveryTag)
        {
            lock (this.thisLock)
            {
                if (this.deliveries.TryGetValue(linkTerminus, out IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries))
                {
                    terminusDeliveries.Remove(deliveryTag);
                }
            }

            return Task.CompletedTask;
        }

        #endregion
    }
}
