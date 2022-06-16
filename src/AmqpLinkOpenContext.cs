// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A context object used for opening or resuming a link.
    /// </summary>
    public class AmqpLinkOpenContext
    {
        /// <summary>
        /// Create a context object used for opening or resuming a link.
        /// </summary>
        /// <param name="linkIdentifier">The link identifier for the new link to be opened or resumed. This property is mandatory.</param>
        public AmqpLinkOpenContext(AmqpLinkIdentifier linkIdentifier)
        {
            if (linkIdentifier == null)
            {
                throw new ArgumentNullException(nameof(linkIdentifier));
            }

            this.LinkIdentifier = linkIdentifier;
        }

        /// <summary>
        /// Required. The link identifier for the new link to be opened or resumed.
        /// </summary>
        public AmqpLinkIdentifier LinkIdentifier { get; }

        /// <summary>
        /// Optional. The initial unsettled deliveries to be set for the link to be opened or resumed. 
        /// If null, unsettled deliveries from existing link terminus with the same link identifier will be used, if there are any.
        /// </summary>
        public IDictionary<ArraySegment<byte>, Delivery> UnsettledDeliveries { get; set; }

        /// <summary>
        /// Optional. The link settings that will be applied to the new link being opened or resumed.
        /// If null, default link settings will be used.
        /// </summary>
        public AmqpLinkSettings LinkSettings { get; set; }

        /// <summary>
        /// Optional. The address that will be applied to the new link being opened or resumed.
        /// If the LinkSettings property is not null and already has Address value, it will override this field.
        /// </summary>
        public string Address { get; set; }
    }
}
