// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Transport;

    public class SaslTransportProvider : TransportProvider
    {
        Dictionary<string, SaslHandler> handlers;

        public SaslTransportProvider()
        {
            this.ProtocolId = ProtocolId.AmqpSasl;
            this.handlers = new Dictionary<string, SaslHandler>();
            this.MaxFrameSize = AmqpConstants.MinMaxFrameSize;
        }

        public int MaxFrameSize
        {
            get;
            set;
        }

        public IEnumerable<string> Mechanisms
        {
            get { return this.handlers.Keys; }
        }

        public void AddHandler(SaslHandler handler)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Add, handler);
            this.handlers.Add(handler.Mechanism, handler);
        }

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

        public override string ToString()
        {
            return "sasl-provider";
        }

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
