// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Timers;

    /// <summary>
    /// A class which represents a link endpoint, which contains information about the link's identifier, settings and a store of unsettled deliveries.
    /// </summary>
    public class AmqpLinkTerminus : IDisposable
    {
        readonly object thisLock;
        bool disposed;
        AmqpLink link;

        /// <summary>
        /// Create a new instance of a link terminus object.
        /// </summary>
        /// <param name="identifier">The identifier that is used to uniquely identify the link endpoint.</param>
        /// <param name="linkSettings">The link settings.</param>
        /// <param name="terminusStore">The <see cref="IAmqpTerminusStore"/> which will be responsible for saving and retrieving LinkTerminus and its associated deliveries.</param>
        public AmqpLinkTerminus(AmqpLinkIdentifier identifier, AmqpLinkSettings linkSettings, IAmqpTerminusStore terminusStore)
        {
            this.thisLock = new object();
            this.Identifier = identifier ?? throw new ArgumentNullException(nameof(identifier));
            this.LinkSettings = linkSettings ?? throw new ArgumentNullException(nameof(linkSettings));
            this.TerminusStore = terminusStore ?? throw new ArgumentNullException(nameof(terminusStore));
        }

        /// <summary>
        /// A handler that will be triggered when this link terminus is suspended due to the corresponding expiry policy.
        /// </summary>
        public event EventHandler Suspended;

        /// <summary>
        /// A handler that will be triggered when this link terminus is expired.
        /// </summary>
        public event EventHandler Expired;

        /// <summary>
        /// Returns the link that's currently associated with the link terminus. 
        /// Please keep in mind that this link may not be the most recent link associated with the terminus, as it could have been stolen.
        /// </summary>
        public AmqpLink Link
        {
            get
            {
                lock (this.thisLock)
                {
                    return this.link;
                }
            }
        }

        /// <summary>
        /// The <see cref="IAmqpTerminusStore"/> which will be responsible for saving and retrieving LinkTerminus and its associated deliveries.
        /// </summary>
        public IAmqpTerminusStore TerminusStore { get; }

        /// <summary>
        /// Returns the identifier used to uniquely identify this terminus object.
        /// </summary>
        public AmqpLinkIdentifier Identifier { get; }

        /// <summary>
        /// Settings of the link with which the linkTerminus was created
        /// </summary>
        public AmqpLinkSettings LinkSettings { get; }

        internal IDictionary<ArraySegment<byte>, Delivery> UnsettledDeliveries { get; set; }

        /// <summary>
        /// The timer object that tracks the expiry of the link terminus after its suspension.
        /// </summary>
        protected Timer ExpireTimer { get; set; }

        /// <summary>
        /// Check if this link terminus object is equal to the other object provided.
        /// </summary>
        /// <param name="other">The other object to check equality against.</param>
        /// <returns>True if the other object is a link terminus object that should be treated as equal to this link terminus object. Please note that they do not have to be the same object reference to be equal.</returns>
        public override bool Equals(object other)
        {
            AmqpLinkTerminus otherLinkTerminus = other as AmqpLinkTerminus;
            return otherLinkTerminus != null && this.Identifier.Equals(otherLinkTerminus.Identifier);
        }

        /// <summary>
        /// Get hash code for this link terminus object.
        /// </summary>
        public override int GetHashCode()
        {
            return this.Identifier.GetHashCode();
        }

        /// <summary>
        /// Return the string representation of this link terminus.
        /// </summary>
        /// <returns>The string representation of this link terminus.</returns>
        public override string ToString()
        {
            return $"{nameof(AmqpLinkTerminus)}({this.Identifier})";
        }

        /// <summary>
        /// Cleanup this link terminus object by cleaning up any timers and events.
        /// </summary>
        public void Dispose()
        {
            lock (this.thisLock)
            {
                if (this.disposed)
                {
                    return;
                }

                this.disposed = true;
            }

            this.link?.Session.Connection.TerminusStore.TryRemoveLinkTerminusAsync(this.Identifier, this).GetAwaiter().GetResult();
            this.DisassociateLink();
        }

        /// <summary>
        /// Process and consolidate the unsettled deliveries sent with the remote Attach frame, by checking against the unsettled deliveries for this link terminus.
        /// </summary>
        /// <param name="remoteAttach">The incoming Attach from remote which contains the remote's unsettled delivery states.</param>
        /// <returns>A task containing the resultant map of deliveries that would remain unsettled.</returns>
        internal protected async Task<IDictionary<ArraySegment<byte>, Delivery>> NegotiateUnsettledDeliveriesAsync(Attach remoteAttach)
        {
            if (this.Identifier.IsReceiver)
            {
                return this.UnsettledDeliveries;
            }
            else
            {
                return await NegotiateUnsettledDeliveriesFromSender(remoteAttach, this.UnsettledDeliveries);
            }
        }

        /// <summary>
        /// Associate a given link with this link terminus.
        /// If there is already a link registered with this link terminus, and it's a different link, the existing link would be closed due to link stealing.
        /// </summary>
        /// <param name="link">The new link to be associated with this link terminus.</param>
        /// <param name="stolenLink">The existing link associated with this link terminus that was disassociated as a result of associating the new link given.</param>
        /// <returns>True if the new link was successfully associated with this link terminus.</returns>
        internal protected bool TryAssociateLink(AmqpLink link, out AmqpLink stolenLink)
        {
            stolenLink = null;
            lock (this.thisLock)
            {
                if (disposed)
                {
                    return false;
                }

                if (this.link != null)
                {
                    if (link.AllowLinkStealing(this.LinkSettings))
                    {
                        stolenLink = this.link;
                        this.DisassociateLink();
                    }
                    else
                    {
                        return false;
                    }
                }

                this.link = link;
                link.Terminus = this;

                link.Closed += this.OnAssociatedLinkClose;
                AmqpSymbol expiryPolicy = link.Settings.GetExpiryPolicy();
                if (expiryPolicy.Equals(TerminusExpiryPolicy.LinkDetach))
                {
                    link.Closed += this.OnSuspendLinkTerminus;
                }
                else if (expiryPolicy.Equals(TerminusExpiryPolicy.SessionEnd))
                {
                    link.Session.Closed += this.OnSuspendLinkTerminus;
                }
                else if (expiryPolicy.Equals(TerminusExpiryPolicy.ConnectionClose))
                {
                    link.Session.Connection.Closed += this.OnSuspendLinkTerminus;
                }
            }

            if (this.UnsettledDeliveries != null)
            {
                if (link.Settings.Unsettled == null)
                {
                    link.Settings.Unsettled = new AmqpMap(new Dictionary<ArraySegment<byte>, Delivery>(), MapKeyByteArrayComparer.Instance);
                }

                foreach (var kvp in UnsettledDeliveries)
                {
                    link.Settings.Unsettled.Add(new MapKey(kvp.Key), kvp.Value.State);
                }
            }

            stolenLink?.OnLinkStolen(false);
            return true;
        }

        /// <summary>
        /// Disassociate the link that's current associated with the link terminus.
        /// </summary>
        protected void DisassociateLink()
        {
            lock (this.thisLock)
            {
                if (this.link != null)
                {
                    this.link.Closed -= this.OnAssociatedLinkClose;
                    AmqpSymbol existingLinkExpiryPolicy = this.Link.Settings.GetExpiryPolicy();
                    if (existingLinkExpiryPolicy.Equals(TerminusExpiryPolicy.LinkDetach) || existingLinkExpiryPolicy.Value == null)
                    {
                        this.link.Closed -= this.OnSuspendLinkTerminus;
                    }
                    else if (existingLinkExpiryPolicy.Equals(TerminusExpiryPolicy.SessionEnd))
                    {
                        this.link.Session.Closed -= this.OnSuspendLinkTerminus;
                    }
                    else if (existingLinkExpiryPolicy.Equals(TerminusExpiryPolicy.ConnectionClose))
                    {
                        this.link.Session.Connection.Closed -= this.OnSuspendLinkTerminus;
                    }

                    this.link.Terminus = null;
                    this.link = null;
                }

                this.ExpireTimer?.Dispose();
            }
        }

        void OnAssociatedLinkClose(object sender, EventArgs e)
        {
            AmqpLink link = (AmqpLink)sender;
            lock (this.thisLock)
            {
                Fx.Assert(link == this.Link, "The closed link should still be associated with this terminus, or else the event handler should have been removed.");
                if (!this.disposed && !link.IsStolen())
                {
                    this.TerminusStore.SaveDeliveriesAsync(this, link.UnsettledMap);
                }
            }
        }

        void OnSuspendLinkTerminus(object sender, EventArgs e)
        {
            ((AmqpObject)sender).Closed -= this.OnSuspendLinkTerminus;
            this.Suspended?.Invoke(this, EventArgs.Empty);
            lock (this.thisLock)
            {
                if (this.ExpireTimer == null)
                {
                    if (this.Link.Settings.GetExpiryTimeout() > TimeSpan.Zero)
                    {
                        this.ExpireTimer = new Timer();
                        this.ExpireTimer.Elapsed += (s, e) => OnExpireLinkTerminus(s, e, this, this.Link);
                        this.ExpireTimer.Interval = this.Link.Settings.GetExpiryTimeout().TotalMilliseconds;
                        this.ExpireTimer.AutoReset = false;
                        this.ExpireTimer.Enabled = true;
                    }
                    else
                    {
                        OnExpireLinkTerminus(null, null, this, this.Link);
                    }
                }
            }
        }

        static void OnExpireLinkTerminus(object sender, ElapsedEventArgs e, AmqpLinkTerminus linkTerminusToExpire, AmqpLink suspendedLink)
        {
            linkTerminusToExpire.Dispose();
            linkTerminusToExpire.Expired?.Invoke(linkTerminusToExpire, EventArgs.Empty);
        }

        async Task<IDictionary<ArraySegment<byte>, Delivery>> NegotiateUnsettledDeliveriesFromSender(Attach remoteAttach, IDictionary<ArraySegment<byte>, Delivery> localUnsettledDeliveries)
        {
            IDictionary<ArraySegment<byte>, Delivery> resultantUnsettledDeliveries = new Dictionary<ArraySegment<byte>, Delivery>();
            if (localUnsettledDeliveries != null)
            {
                foreach (Delivery localSenderDelivery in localUnsettledDeliveries.Values)
                {
                    ArraySegment<byte> deliveryTag = localSenderDelivery.DeliveryTag;
                    DeliveryState localDeliveryState = localSenderDelivery.State;
                    DeliveryState peerDeliveryState = null;
                    bool peerHasDelivery = remoteAttach.Unsettled?.TryGetValue(new MapKey(deliveryTag), out peerDeliveryState) == true;
                    bool remoteReachedTerminal = peerDeliveryState.IsTerminal();

                    if (localDeliveryState == null || localDeliveryState.DescriptorCode == Received.Code)
                    {
                        // This section takes care of OASIS AMQP doc section 3.4.6, examples 1 to 8.
                        // Currently we do not support partial delivery for Received delivery states, so just resend the whole delivery anyways.

                        // Currently we do not support partial delivery for local Received delivery states, such as OASIS AMQP doc section 3.4.6 examples 6, 7, 9.
                        // In this case, mark the delivery as aborted, then resend the delivery later if possible.
                        localSenderDelivery.Aborted = peerDeliveryState.Transactional() ||
                            (localDeliveryState?.DescriptorCode == Received.Code && peerHasDelivery && (peerDeliveryState == null || peerDeliveryState.DescriptorCode == Received.Code));

                        // If the remote has reached terminal outcome and local has not, simply apply the outcome to local and respond with the outcome.
                        // This would be seen by OASIS AMQP doc section 3.4.6 examples 3, 8.
                        if (remoteReachedTerminal)
                        {
                            localSenderDelivery.State = peerDeliveryState;
                            localSenderDelivery.Settled = true;
                        }
                    }
                    else if (localDeliveryState.IsTerminal())
                    {
                        // This section takes care of OASIS AMQP doc section 3.4.6, examples 10 to 14.
                        if (!peerHasDelivery)
                        {
                            // This is OASIS AMQP doc section 3.4.6 example 10, where the delivery does not need to be resent. Simply settle and remove it locally.
                            await this.TerminusStore.RemoveDeliveryAsync(this, deliveryTag);
                            continue;
                        }
                        else
                        {
                            if (remoteReachedTerminal)
                            {
                                // OASIS AMQP doc section 3.4.6, delivery 12, 13.
                                localSenderDelivery.Settled = peerDeliveryState.DescriptorCode == localDeliveryState.DescriptorCode;
                            }
                            else
                            {
                                // OASIS AMQP doc section 3.4.6, delivery 11, 14.
                                localSenderDelivery.Aborted = true;
                            }
                        }
                    }
                    else if (localDeliveryState.Transactional())
                    {
                        Outcome localTransactionalOutcome = ((TransactionalState)localDeliveryState).Outcome;

                        if (peerDeliveryState.Transactional())
                        {
                            Outcome remoteTransactionalOutcome = ((TransactionalState)peerDeliveryState).Outcome;
                            if (!remoteTransactionalOutcome.IsTerminal())
                            {
                                // If the remote has not reached terminal outcome, just abort the delivery to avoid any in-doubt states with the transaction,
                                // since we are not sure what kind of state the remote is currently in (Transactional does convey states like "Received" state).
                                localSenderDelivery.Aborted = true;
                            }
                            else
                            {
                                if (!localTransactionalOutcome.IsTerminal())
                                {
                                    // If the remote receiver has already reached terminal outcome, just apply that to the local sender as well, similar to OASIS AMQP doc section 3.4.6 examples 3, 8.
                                    ((TransactionalState)localDeliveryState).Outcome = remoteTransactionalOutcome;
                                    localSenderDelivery.Settled = true;
                                }
                                else
                                {
                                    // OASIS AMQP doc section 3.4.6, delivery 12, 13.
                                    localSenderDelivery.Settled = remoteTransactionalOutcome.DescriptorCode == localTransactionalOutcome.DescriptorCode;
                                }
                            }
                        }
                        else if (!peerHasDelivery)
                        {
                            if (localTransactionalOutcome.IsTerminal())
                            {
                                // This is OASIS AMQP doc section 3.4.6 example 10, where the delivery does not need to be resent. Simply settle and remove it locally.
                                await this.TerminusStore.RemoveDeliveryAsync(this, deliveryTag);
                                continue;
                            }
                            else
                            {
                                // Resend the delivery as new, similar to OASIS AMQP doc section 3.4.6 example 1.
                            }
                        }
                        else
                        {
                            // This is unexpected scenario, where only local sender delivery is transactional, just abort it to be safe.
                            localSenderDelivery.Aborted = true;
                        }
                    }
                    else
                    {
                        Fx.Assert(false, $"Unexpected deliveriy state. Local sender DeliveryState: {localDeliveryState}, Remote receiver DeliveryState: {peerDeliveryState}");
                        localSenderDelivery.Aborted = true;
                    }

                    localSenderDelivery.Resume = peerHasDelivery;
                    resultantUnsettledDeliveries.Add(localSenderDelivery.DeliveryTag, localSenderDelivery);
                    if (localSenderDelivery.Settled)
                    {
                        await this.TerminusStore.RemoveDeliveryAsync(this, localSenderDelivery.DeliveryTag);
                    }
                }
            }

            return resultantUnsettledDeliveries;
        }
    }
}
