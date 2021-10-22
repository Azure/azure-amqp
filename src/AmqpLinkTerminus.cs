namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Framing;
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A class which represents a link endpoint, which contains information about the link's settings and unsettled map.
    /// This may be used for link recovery.
    /// </summary>
    public class AmqpLinkTerminus
    {
        internal AmqpLinkTerminus(AmqpLinkSettings settings)
        {
            if (settings == null)
            {
                throw new ArgumentNullException(nameof(settings));
            }

            this.Settings = settings;
            this.UnsettledMap = new Dictionary<ArraySegment<byte>, Delivery>(ByteArrayComparer.Instance);
        }

        internal string ContainerId { get; set; }

        internal AmqpLinkSettings Settings { get; set; }

        internal string RemoteContainerId { get; set; }

        internal Dictionary<ArraySegment<byte>, Delivery> UnsettledMap { get; set; }

        /// <summary>
        /// Determines whether two link termini are equal based on <see cref="Attach.LinkName"/>
        /// and <see cref="Attach.Role"/> of their link settings. Name comparison is case insensitive.
        /// </summary>
        /// <param name="obj">The object to compare with the current object.</param>
        public override bool Equals(object obj)
        {
            AmqpLinkTerminus other = obj as AmqpLinkTerminus;
            return this.Settings.Equals(other.Settings);
        }

        /// <summary>
        /// Gets a hash code of the object.
        /// </summary>
        public override int GetHashCode()
        {
            return this.Settings.GetHashCode();
        }
    }
}
