// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using System;
    using System.Collections.Generic;
    using System.Timers;

    /// <summary>
    /// A class which represents a link endpoint, which contains information about the link's settings and unsettled map.
    /// This class should be treated as a "snapshot" of the link's current state and may be used for link recovery.
    /// </summary>
    public class AmqpLinkTerminus : IDisposable
    {
        object thisLock = new object();
        bool disposed;
        AmqpLink link;
        Timer expireTimer;

        /// <summary>
        /// Create a new instance of a link terminus object.
        /// </summary>
        /// <param name="identifier">The identifier that is used to uniquely identify the link endpoint.</param>
        /// <param name="deliveryStore">The <see cref="IAmqpDeliveryStore"/> which will be responsible for getting and saving unsettled deliveires for this link terminus.</param>
        public AmqpLinkTerminus(AmqpLinkIdentifier identifier, IAmqpDeliveryStore deliveryStore)
        {
            if (identifier == null)
            {
                throw new ArgumentNullException(nameof(identifier));
            }

            if (deliveryStore == null)
            {
                throw new ArgumentNullException(nameof(deliveryStore));
            }

            this.Identifier = identifier;
            this.DeliveryStore = deliveryStore;
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
        /// The <see cref="IAmqpDeliveryStore"/> which will be responsible for getting and saving unsettled deliveires for this link terminus.
        /// </summary>
        public IAmqpDeliveryStore DeliveryStore { get; }

        /// <summary>
        /// Returns the identifier used to uniquely identify this terminus object.
        /// </summary>
        public AmqpLinkIdentifier Identifier { get; }

        IAmqpLinkTerminusManager TerminusManager => this.Link?.Session.Connection.LinkTerminusManager;

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

            this.DeliveryStore.RemoveDeliveriesAsync(this);
            this.link?.Session.Connection.LinkTerminusManager.TryRemoveLinkTerminus(new KeyValuePair<AmqpLinkIdentifier, AmqpLinkTerminus>(this.Identifier, this));
            this.DisassociateLink();
        }

        /// <summary>
        /// Associate a given link with this link terminus.
        /// If there is already a link registered with this link terminus, and it's a different link, the existing link would be closed due to link stealing.
        /// </summary>
        /// <param name="link">The new link to be associated with this link terminus.</param>
        /// <returns>True if the new link was successfully associated with this link terminus.</returns>
        internal bool TryAssociateLink(AmqpLink link)
        {
            AmqpLink stolenLink = null;
            lock (this.thisLock)
            {
                if (disposed)
                {
                    return false;
                }

                if (this.Link != null)
                {
                    if (this.Link.AllowLinkStealing(link))
                    {
                        stolenLink = this.Link;
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

            stolenLink?.OnLinkStolen(false);
            return true;
        }

        void DisassociateLink()
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

                this.expireTimer?.Dispose();
            }
        }

        void OnAssociatedLinkClose(object sender, EventArgs e)
        {
            AmqpLink link = (AmqpLink)sender;
            lock (this.thisLock)
            {
                Fx.Assert(link == this.Link, "The closed link should still be associated with this terminus, or else the event handler should have been removed.");
                if (!this.disposed)
                {
                    foreach (var kvp in link.UnsettledMap)
                    {
                        this.DeliveryStore.SaveDeliveryAsync(this, kvp.Value);
                    }
                }
            }
        }

        void OnSuspendLinkTerminus(object sender, EventArgs e)
        {
            ((AmqpObject)sender).Closed -= this.OnSuspendLinkTerminus;
            this.Suspended?.Invoke(this, EventArgs.Empty);
            lock (this.thisLock)
            {
                if (this.expireTimer == null)
                {
                    if (this.Link.Settings.GetExpiryTimeout() > TimeSpan.Zero)
                    {
                        this.expireTimer = new Timer();
                        this.expireTimer.Elapsed += (s, e) => OnExpireLinkTerminus(s, e, this, this.Link);
                        this.expireTimer.Interval = this.Link.Settings.GetExpiryTimeout().TotalMilliseconds;
                        this.expireTimer.AutoReset = false;
                        this.expireTimer.Enabled = true;
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
    }
}
