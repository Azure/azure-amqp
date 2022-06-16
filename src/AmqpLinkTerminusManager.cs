// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

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
        /// Negotiate and consolidate the unsettled deliveries map from the remote Attach and the local link terminus associated with the given link identifier.
        /// </summary>
        /// <param name="linkIdentifier">The link identifier used to find the associated local link terminus.</param>
        /// <param name="remoteAttach">The <see cref="Attach"/> received from remote as part of a link open process.</param>
        /// <returns>
        /// A task containing a map of deliveries which should remain to be unsettled for the local link terminus.
        /// For senders, this means that these unsettled deliveries may need to be redelivered to remote.
        /// For receivers, this means that these unsettled deliveries have already been previously delivered, so they may be skipped or directly settled in case of redelivery from remote sender.
        /// </returns>
        public async Task<IDictionary<ArraySegment<byte>, Delivery>> ProcessRemoteUnsettledMapAsync(AmqpLinkIdentifier linkIdentifier, Attach remoteAttach)
        {
            IDictionary<ArraySegment<byte>, Delivery> resultantUnsettledDeliveries = new Dictionary<ArraySegment<byte>, Delivery>();
            AmqpLinkTerminus localTerminus = null;
            lock (this.linkTerminiLock)
            {
                this.linkTermini.TryGetValue(linkIdentifier, out localTerminus);
            }

            if (localTerminus != null)
            {
                if (linkIdentifier.IsReceiver)
                {
                    resultantUnsettledDeliveries = await ProcessRemoteUnsettledDeliveriesForReceiversAsync(localTerminus, remoteAttach);
                }
                else
                {
                    resultantUnsettledDeliveries = await ProcessRemoteUnsettledDeliveriesForSendersAsync(localTerminus, remoteAttach);
                }
            }

            return resultantUnsettledDeliveries;
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

        ///// <summary>
        ///// Register the given link with the given unsettled deliveries by checking if there is any existing link terminus objects to associate the link with, or create a new link terminus for it.
        ///// </summary>
        ///// <param name="link">The link to be tracked.</param>
        ///// <param name="unsettledMap">The map of unsettled deliveries that the given link is expected to have.</param>
        //public void RegisterLink(AmqpLink link, IDictionary<ArraySegment<byte>, Delivery> unsettledMap)
        //{
        //    Fx.Assert(link != null, "Should not be registering a null link.");

        //    AmqpLinkTerminus linkTerminus;
        //    AmqpLinkTerminus existingLinkTerminus;
        //    lock (this.linkTerminiLock)
        //    {
        //        if (this.TryGetLinkTerminus(link.LinkIdentifier, out existingLinkTerminus))
        //        {
        //            linkTerminus = existingLinkTerminus;
        //        }
        //        else
        //        {
        //            linkTerminus = new AmqpLinkTerminus(link.LinkIdentifier, link.OpenContext?.UnsettledDeliveries);
        //            this.linkTermini.Add(link.LinkIdentifier, linkTerminus);
        //        }
        //    }

        //    linkTerminus.AssociateLink(link);
        //}

        static async Task<IDictionary<ArraySegment<byte>, Delivery>> ProcessRemoteUnsettledDeliveriesForReceiversAsync(AmqpLinkTerminus localReceiverTerminus, Attach attach)
        {
            IDictionary<ArraySegment<byte>, Delivery> localReceiverUnsettledDeliveries = await localReceiverTerminus.DeliveryStore.RetrieveDeliveriesAsync(localReceiverTerminus);
            IDictionary<ArraySegment<byte>, Delivery> resultantUnsettledDeliveries = new Dictionary<ArraySegment<byte>, Delivery>();

            if (localReceiverUnsettledDeliveries != null)
            {
                foreach (Delivery localReceiverDelivery in localReceiverUnsettledDeliveries.Values)
                {
                    ArraySegment<byte> deliveryTag = localReceiverDelivery.DeliveryTag;
                    DeliveryState localDeliveryState = localReceiverDelivery.State;
                    DeliveryState peerDeliveryState = null;
                    bool peerHasDelivery = attach.Unsettled?.TryGetValue(new MapKey(deliveryTag), out peerDeliveryState) == true;

                    // If the delivery has reached terminal outcome on both sides, it means that the deliery has been already processed.
                    // We should mark them on the receiver side so when the sender sends them again, we can simply settle them and skip processing.
                    if (localDeliveryState.IsTerminal() && peerDeliveryState.IsTerminal())
                    {
                        if (localReceiverUnsettledDeliveries.TryGetValue(deliveryTag, out Delivery delivery) && !delivery.Settled)
                        {
                            resultantUnsettledDeliveries.Add(deliveryTag, delivery);
                        }
                    }
                }
            }

            return resultantUnsettledDeliveries;
        }

        /// <summary>
        /// Process the remote unsettled deliveries when the local side is sender.
        /// </summary>
        /// <param name="localSenderTerminus">The link terminus for the local side sender.</param>
        /// <param name="attach">The incoming Attach from remote.</param>
        /// <returns>A task containing the resultant map of deliveries that would remain unsettled for the local sender.</returns>
        static async Task<IDictionary<ArraySegment<byte>, Delivery>> ProcessRemoteUnsettledDeliveriesForSendersAsync(AmqpLinkTerminus localSenderTerminus, Attach attach)
        {
            IDictionary<ArraySegment<byte>, Delivery> localSenderUnsettledDeliveries = await localSenderTerminus.DeliveryStore.RetrieveDeliveriesAsync(localSenderTerminus);
            IDictionary<ArraySegment<byte>, Delivery> resultantUnsettledDeliveries = new Dictionary<ArraySegment<byte>, Delivery>();
            if (localSenderUnsettledDeliveries != null)
            {
                foreach (Delivery localSenderDelivery in localSenderUnsettledDeliveries.Values)
                {
                    ArraySegment<byte> deliveryTag = localSenderDelivery.DeliveryTag;
                    DeliveryState localDeliveryState = localSenderDelivery.State;
                    DeliveryState peerDeliveryState = null;
                    bool peerHasDelivery = attach.Unsettled?.TryGetValue(new MapKey(deliveryTag), out peerDeliveryState) == true;
                    bool remoteReachedTerminal = peerDeliveryState.IsTerminal();
                    bool alreadySettledOnSend = localSenderTerminus.Link.Settings.SettleType == SettleMode.SettleOnSend && (!peerHasDelivery || (localDeliveryState == null && peerDeliveryState == null));

                    if (localDeliveryState == null || localDeliveryState is Received)
                    {
                        // remoteReachedTerminal: OASIS AMQP doc section 3.4.6, delivery 3, 8
                        // alreadySettledOnSend: OASIS AMQP doc section 3.4.6, delivery 1, 4, 5
                        if (!remoteReachedTerminal && !alreadySettledOnSend)
                        {
                            // currently we don't support resume sending partial payload with Received state.
                            localSenderDelivery.State = null;

                            // abort the delivery if the remote receiver somehow has it as transactional
                            // or this sender has only delivered partially, OASIS AMQP doc section 3.4.6, delivery 6, 7, 9.
                            localSenderDelivery.Aborted = peerDeliveryState.Transactional() || (localDeliveryState?.DescriptorCode == Received.Code && peerHasDelivery);
                        }
                    }
                    else if (localDeliveryState.IsTerminal())
                    {
                        if (peerHasDelivery)
                        {
                            if (remoteReachedTerminal)
                            {
                                // OASIS AMQP doc section 3.4.6, delivery 12, 13.
                                localSenderDelivery.Settled = peerDeliveryState.DescriptorCode == localDeliveryState.DescriptorCode;
                            }
                            else
                            {
                                // OASIS AMQP doc section 3.4.6, delivery 11, 14, or when remote receiver is Transactional
                                localSenderDelivery.Aborted = true;
                            }
                        }
                        else
                        {
                            // OASIS AMQP doc section 3.4.6, delivery 10. Do nothing.
                        }
                    }
                    else if (localDeliveryState is TransactionalState localTransactionalState)
                    {
                        if (localTransactionalState.Outcome != null)
                        {
                            if (!peerHasDelivery)
                            {
                                // no need to resend the delivery if local sender has reached terminal state and remote receiver does not have this deliery, similar to OASIS AMQP doc section 3.4.6, delivery 10.
                                continue;
                            }
                            else if (peerDeliveryState is TransactionalState peerTransactionalState && peerTransactionalState.Outcome != null)
                            {
                                // both the sender and receiver reached transactional terminal state, similar to OASIS AMQP doc section 3.4.6, delivery 12, 13.
                                localSenderDelivery.Settled = peerTransactionalState.Outcome.GetType() == localTransactionalState.Outcome.GetType();
                            }
                            else
                            {
                                // the receiver side is not in a transactional state, which should not happen because the sender is in transactional state.
                                // OR, the receiver side is also in a transactional state but has not reached terminal outcome, similar to OASIS AMQP doc section 3.4.6, delivery 11, 14.
                                localSenderDelivery.Aborted = true;
                            }
                        }
                        else
                        {
                            if (!peerHasDelivery)
                            {
                                // The receiver does not have any record of the delivery, similar to OASIS AMQP doc section 3.4.6, delivery 1.
                                if (alreadySettledOnSend)
                                {
                                    continue;
                                }
                            }
                            else if (peerDeliveryState is TransactionalState peerTransactionalState && peerTransactionalState.Outcome != null)
                            {
                                // the receiver has already reached terminal oucome, no need to resend, similar to OASIS AMQP doc section 3.4.6, delivery 3, 8
                                continue;
                            }
                            else
                            {
                                // the receiver has not reached terminal outcome and may or may not have received the whole delivery. Abort to be safe.
                                localSenderDelivery.Aborted = true;
                            }
                        }
                    }

                    if (localSenderDelivery != null)
                    {
                        localSenderDelivery.Resume = peerHasDelivery;
                        resultantUnsettledDeliveries.Add(localSenderDelivery.DeliveryTag, localSenderDelivery);
                    }
                }
            }

            return resultantUnsettledDeliveries;
        }
    }
}
