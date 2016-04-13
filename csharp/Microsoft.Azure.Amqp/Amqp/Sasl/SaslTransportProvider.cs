// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;
    using Microsoft.Azure.Amqp.Tracing;

    public sealed class SaslTransportProvider : TransportProvider
    {
        Dictionary<string, SaslHandler> handlers;

        public SaslTransportProvider()
        {
            this.ProtocolId = ProtocolId.AmqpSasl;
            this.handlers = new Dictionary<string, SaslHandler>();
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
                AmqpTrace.Provider.AmqpLogError(this, "GetHandler", mechanism);
                throw new AmqpException(AmqpErrorCode.NotImplemented, mechanism);
            }

            return clone ? handler.Clone() : handler;
        }

        public override string ToString()
        {
            return "sasl-provider";
        }

        protected override TransportBase OnCreateTransport(TransportBase innerTransport, bool isInitiator)
        {
            return new SaslTransport(innerTransport, this, isInitiator);
        }
    }
}
