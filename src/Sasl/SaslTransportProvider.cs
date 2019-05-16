// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// Provides SASL transport upgrade.
    /// </summary>
    public class SaslTransportProvider : TransportProvider
    {
        Dictionary<string, SaslHandler> handlers;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslTransportProvider()
        {
            this.ProtocolId = ProtocolId.AmqpSasl;
            this.handlers = new Dictionary<string, SaslHandler>();
            this.MaxFrameSize = AmqpConstants.MinMaxFrameSize;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslTransportProvider(AmqpVersion version)
            : this()
        {
            this.Versions.Add(version);
        }

        /// <summary>
        /// Gets or sets the max-frame-size of SASL frames.
        /// </summary>
        public int MaxFrameSize
        {
            get;
            set;
        }

        /// <summary>
        /// The supported SASL mechanisms.
        /// </summary>
        public IEnumerable<string> Mechanisms
        {
            get { return this.handlers.Keys; }
        }

        /// <summary>
        /// Adds a SASL handler.
        /// </summary>
        /// <param name="handler">The SASL handler.</param>
        public void AddHandler(SaslHandler handler)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Add, handler);
            this.handlers.Add(handler.Mechanism, handler);
        }

        /// <summary>
        /// Gets a SASL handler for a given mechanism.
        /// </summary>
        /// <param name="mechanism">The SASL mechanism.</param>
        /// <param name="clone">true to clone the handler.</param>
        /// <returns></returns>
        public SaslHandler GetHandler(string mechanism, bool clone)
        {
            SaslHandler handler;
            if (!this.handlers.TryGetValue(mechanism, out handler))
            {
                var exception = new AmqpException(AmqpErrorCode.NotImplemented, mechanism);
                AmqpTrace.Provider.AmqpLogError(this, "GetHandler", exception);
                throw exception;
            }

            return clone ? handler.Clone() : handler;
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return "sasl-provider";
        }

        /// <summary>
        /// Creates a SASL transport from the inner transport.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="isInitiator">true if it is called by the transport initiator.</param>
        /// <returns></returns>
        protected override TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            if (innerTransport.GetType() == typeof(SaslTransport))
            {
                throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpTransportUpgradeNotAllowed,
                    innerTransport.GetType().Name, typeof(SaslTransport).Name));
            }

            return new SaslTransport(innerTransport, this, isInitiator);
        }
    }
}
