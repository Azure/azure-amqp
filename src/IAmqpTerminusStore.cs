// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// This class should be responsible for 
    /// 1. Storage and retrieval of LinkTerminus (either in-memory or disk).
    /// 2. Storage and retrieval of deliveries associated with the LinkTerminus (either in-memory or disk).
    /// </summary>
    public interface IAmqpTerminusStore
    {
        #region TerminusManagement

        /// <summary>
        /// Try to add a link terminus object which will be identified by the link identifier if there is no existing link terminus associated to the given link identifier.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object to be added.</param>
        /// <returns>True if it was successfully added, or false if there is already an existing entry and the value is not added.</returns>
        Task<bool> TryAddLinkTerminusAsync(AmqpLinkIdentifier linkIdentifier, AmqpLinkTerminus linkTerminus);

        /// <summary>
        /// Remove the link terminus object that's identified by the given link identifier if there is any entry that matches both the key and value.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used as a key to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object to be removed.</param>
        /// <returns>True if the linkTerminus object was successfully found and removed, false otherwise.</returns>
        Task<bool> TryRemoveLinkTerminusAsync(AmqpLinkIdentifier linkIdentifier, AmqpLinkTerminus linkTerminus);

        /// <summary>
        /// Try to get the link terminus with the given link identifier. Return true if the link terminus has been found.
        /// </summary>
        /// <param name="linkIdentifier">The unique identifier of a link endpoint which will be used to identify the link terminus.</param>
        /// <param name="linkTerminus">The link terminus object associated with the link identifier.</param>
        /// <returns>True if there is a link terminus object was found associated with the given link identifier.</returns>
        Task<bool> TryGetLinkTerminusAsync(AmqpLinkIdentifier linkIdentifier, out AmqpLinkTerminus linkTerminus);

        #endregion

        #region DeliveryManagement

        /// <summary>
        /// Save a collection of <see cref="Delivery"/> under the given link terminus to be retrieved later.
        /// </summary>
        /// <param name="linkTerminus">The link terminus associated with the deliveries.</param>
        /// <param name="unsettledDeliveries">A dictionary of Key-Value pairs of DeliveryTag and corresponding delivery.</param>
        /// <returns>True if the deliveries were successfully saved under the link terminus.</returns>
        Task SaveDeliveriesAsync(AmqpLinkTerminus linkTerminus, IDictionary<ArraySegment<byte>, Delivery> unsettledDeliveries);

        /// <summary>
        /// Try to remove a delivery that's stored under the given link terminus with the given delivery tag, if any.
        /// </summary>
        /// <param name="linkTerminus">The link terminus that the removed delivery belongs to.</param>
        /// <param name="deliveryTag">The delivery tag of the delivery to be removed.</param>
        /// <returns>True if the delivery was found and removed, false otherwise.</returns>
        Task RemoveDeliveryAsync(AmqpLinkTerminus linkTerminus, ArraySegment<byte> deliveryTag);

        #endregion
    }
}
