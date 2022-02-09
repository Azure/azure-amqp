namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A class which represents a link endpoint, which contains information about the link's settings and unsettled map.
    /// This class should be treated as a "snapshot" of the link's current state and may be used for link recovery.
    /// </summary>
    public class AmqpLinkTerminus
    {
        internal AmqpLinkTerminus(AmqpLinkSettings settings, Dictionary<ArraySegment<byte>, Delivery> unsettledMap)
        {
            if (settings == null)
            {
                throw new ArgumentNullException(nameof(settings));
            }

            this.Settings = settings;
            this.UnsettledMap = unsettledMap;
        }

        internal AmqpLinkSettings Settings { get; }

        internal Dictionary<ArraySegment<byte>, Delivery> UnsettledMap { get; }
    }
}
