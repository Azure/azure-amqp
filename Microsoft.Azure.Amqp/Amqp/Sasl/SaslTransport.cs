// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Security.Principal;
    using Microsoft.Azure.Amqp.Transport;

    public sealed class SaslTransport : TransportBase
    {
        readonly TransportBase innerTransport;
        SaslNegotiator negotiator;

        public SaslTransport(TransportBase transport, SaslTransportProvider provider, bool isInitiator)
            : base("sasl", transport.Identifier)
        {
            this.innerTransport = transport;
            this.negotiator = new SaslNegotiator(this, provider, isInitiator);
        }

        public override string LocalEndPoint
        {
            get
            {
                return this.innerTransport.LocalEndPoint;
            }
        }

        public override string RemoteEndPoint
        {
            get
            {
                return this.innerTransport.RemoteEndPoint;
            }
        }

        public override bool IsSecure
        {
            get { return this.innerTransport.IsSecure; }
        }

        public override bool RequiresCompleteFrames
        {
            get { return this.innerTransport.RequiresCompleteFrames; }
        }

        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.innerTransport.SetMonitor(usageMeter);
        }

        public override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            return this.innerTransport.WriteAsync(args);
        }

        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            return this.innerTransport.ReadAsync(args);
        }

        public void OnNegotiationSucceed(IPrincipal principal)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "OnNegotiationSucceed");
            this.negotiator = null;
            this.Principal = principal;
            this.CompleteOpen(false, null);
        }

        public void OnNegotiationFail(Exception exception)
        {
            AmqpTrace.Provider.AmqpLogError(this, "OnNegotiationFail", exception.Message);
            this.negotiator = null;
            this.innerTransport.SafeClose(exception);
            this.CompleteOpen(false, exception);
        }

        protected override bool OpenInternal()
        {
            return this.negotiator.Start();
        }

        protected override void AbortInternal()
        {
            this.innerTransport.Abort();
        }

        protected override bool CloseInternal()
        {
            this.innerTransport.Close();
            return true;
        }
    }
}
