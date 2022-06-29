// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// This class will be used to save and retrieve deliveries in memory.
    /// </summary>
    public class AmqpInMemoryDeliveryStore : IAmqpDeliveryStore
    {
        IDictionary<AmqpLinkTerminus, IDictionary<ArraySegment<byte>, Delivery>> deliveries;
        object thisLock;

        /// <summary>
        /// Create an object that will be used to save and retrieve deliveries in memory.
        /// </summary>
        public AmqpInMemoryDeliveryStore()
        {
            this.deliveries = new Dictionary<AmqpLinkTerminus, IDictionary<ArraySegment<byte>, Delivery>>();
            this.thisLock = new object();
        }

        /// <summary>
        /// Retrieve a specific <see cref="Delivery"/> with the given link terminus and delivery tag.
        /// </summary>
        /// <param name="linkTerminus">The <see cref="AmqpLinkTerminus"/> that the delivery belongs to.</param>
        /// <param name="deliveryTag">The delivery tag that identifies the delivery.</param>
        /// <returns>A task containing the retreived delivery.</returns>
        public Task<Delivery> RetrieveDeliveryAsync(AmqpLinkTerminus linkTerminus, ArraySegment<byte> deliveryTag)
        {
            Delivery delivery = null;
            lock (this.thisLock)
            {
                if (this.deliveries.TryGetValue(linkTerminus, out IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries))
                {
                    terminusDeliveries.TryGetValue(deliveryTag, out delivery);
                }
            }

            return Task.FromResult(delivery);
        }

        /// <summary>
        /// Retrieve saved deliveries to the given link terminus.
        /// </summary>
        /// <param name="linkTerminus">The <see cref="AmqpLinkTerminus"/> to retrieve deliveries for.</param>
        /// <returns>A task containing the retreived deliveries.</returns>
        public Task<IDictionary<ArraySegment<byte>, Delivery>> RetrieveDeliveriesAsync(AmqpLinkTerminus linkTerminus)
        {
            lock (this.thisLock)
            {
                if (this.deliveries.TryGetValue(linkTerminus, out IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries))
                {
                    return Task.FromResult<IDictionary<ArraySegment<byte>, Delivery>>(new Dictionary<ArraySegment<byte>, Delivery>(terminusDeliveries));
                }
            }

            return Task.FromResult<IDictionary<ArraySegment<byte>, Delivery>>(new Dictionary<ArraySegment<byte>, Delivery>());
        }

        /// <summary>
        /// Save a <see cref="Delivery"/> instance under the given link terminus to be retrieved later.
        /// If there is already an existing Delivery saved under the same link terminus, override it.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the delivery belongs to.</param>
        /// <param name="delivery">The delivery object to be saved.</param>
        /// <returns>True if the delivery was successfully saved under the link terminus.</returns>
        public Task SaveDeliveryAsync(AmqpLinkTerminus linkTerminus, Delivery delivery)
        {
            if (delivery == null)
            {
                throw new ArgumentNullException(nameof(delivery));
            }

            lock (this.thisLock)
            {
                IDictionary<ArraySegment<byte>, Delivery> terminusDeliveries;
                if (!this.deliveries.TryGetValue(linkTerminus, out terminusDeliveries))
                {
                    terminusDeliveries = new Dictionary<ArraySegment<byte>, Delivery>();
                    this.deliveries.Add(linkTerminus, terminusDeliveries);
                }

                terminusDeliveries.Remove(delivery.DeliveryTag);
                terminusDeliveries.Add(delivery.DeliveryTag, delivery);
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

        /// <summary>
        /// Try to remove all deliveries that are stored under the given link terminus, if any.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the removed deliveries belongs to.</param>
        /// <returns>True if the deliveries were found and removed, false otherwise.</returns>
        public Task RemoveDeliveriesAsync(AmqpLinkTerminus linkTerminus)
        {
            lock (this.thisLock)
            {
                this.deliveries.Remove(linkTerminus);
            }

            return Task.CompletedTask;
        }
    }
}
