// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// This class should be responsible for the storage and retrival of deliveries (in disk, in memory, etc.)
    /// </summary>
    public interface IAmqpDeliveryStore
    {
        /// <summary>
        /// Retrieve a specific <see cref="Delivery"/> with the given link terminus and delivery tag.
        /// </summary>
        /// <param name="linkTerminus">The <see cref="AmqpLinkTerminus"/> that the delivery belongs to.</param>
        /// <param name="deliveryTag">The delivery tag that identifies the delivery.</param>
        /// <returns>A task containing the retreived delivery.</returns>
        Task<Delivery> RetrieveDeliveryAsync(AmqpLinkTerminus linkTerminus, ArraySegment<byte> deliveryTag);

        /// <summary>
        /// Retrieve saved deliveries to the given link terminus.
        /// </summary>
        /// <param name="linkTerminus">The <see cref="AmqpLinkTerminus"/> to retrieve deliveries for.</param>
        /// <returns>A task containing the retreived deliveries.</returns>
        Task<IDictionary<ArraySegment<byte>, Delivery>> RetrieveDeliveriesAsync(AmqpLinkTerminus linkTerminus);

        /// <summary>
        /// Save a <see cref="Delivery"/> instance under the given link terminus to be retrieved later.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the delivery belongs to.</param>
        /// <param name="delivery">The delivery object to be saved.</param>
        /// <returns>True if the delivery was successfully saved under the link terminus.</returns>
        Task SaveDeliveryAsync(AmqpLinkTerminus linkTerminus, Delivery delivery);

        /// <summary>
        /// Try to remove a delivery that's stored under the given link terminus with the given delivery tag, if any.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the removed delivery belongs to.</param>
        /// <param name="deliveryTag">The delivery tag of the delivery to be removed.</param>
        /// <returns>True if the delivery was found and removed, false otherwise.</returns>
        Task RemoveDeliveryAsync(AmqpLinkTerminus linkTerminus, ArraySegment<byte> deliveryTag);

        /// <summary>
        /// Try to remove all deliveries that are stored under the given link terminus, if any.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the removed deliveries belongs to.</param>
        /// <returns>True if the deliveries were found and removed, false otherwise.</returns>
        Task RemoveDeliveriesAsync(AmqpLinkTerminus linkTerminus);
    }
}
