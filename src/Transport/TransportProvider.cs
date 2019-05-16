// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp;

    /// <summary>
    /// Defines the base class of a transport provider.
    /// </summary>
    public abstract class TransportProvider
    {
        List<AmqpVersion> versions;

        /// <summary>
        /// Gets or sets the protocol ID.
        /// </summary>
        public ProtocolId ProtocolId
        {
            get;
            protected set;
        }

        /// <summary>
        /// Gets the supported versions in preferred order.
        /// </summary>
        public IList<AmqpVersion> Versions
        {
            get
            {
                if (this.versions == null)
                {
                    this.versions = new List<AmqpVersion>();
                }

                return this.versions;
            }
        }

        /// <summary>
        /// Gets the default version.
        /// </summary>
        public AmqpVersion DefaultVersion
        {
            get 
            {
                if (this.Versions.Count == 0)
                {
                    throw new ArgumentException(AmqpResources.GetString(AmqpResources.AmqpProtocolVersionNotSet, this));
                }

                return this.Versions[0]; 
            }
        }

        /// <summary>
        /// Examines if the request version is supported.
        /// </summary>
        /// <param name="requestedVersion">The requested version.</param>
        /// <param name="supportedVersion">The supported version.</param>
        /// <returns>true if the requested version is supported, otherwise false.</returns>
        public bool TryGetVersion(AmqpVersion requestedVersion, out AmqpVersion supportedVersion)
        {
            supportedVersion = this.DefaultVersion;
            foreach (AmqpVersion version in this.Versions)
            {
                if (version.Equals(requestedVersion))
                {
                    supportedVersion = requestedVersion;
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Creates a transport from the inner transport.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="isInitiator">true if the caller is the transport initiator, otherwise false.</param>
        /// <returns>The transport.</returns>
        public TransportBase CreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            return this.OnCreateTransport(innerTransport, isInitiator);
        }

        /// <summary>
        /// When overriden in derived classes, creates a transport from the inner transport.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="isInitiator">true if the caller is the transport initiator, otherwise false.</param>
        /// <returns>The transport.</returns>
        protected abstract TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator);
    }
}
