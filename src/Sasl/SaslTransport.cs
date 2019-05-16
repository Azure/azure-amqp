// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Net;
    using System.Security.Principal;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// Defines the SASL transport.
    /// </summary>
    public class SaslTransport : TransportBase
    {
        readonly TransportBase innerTransport;
        SaslNegotiator negotiator;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="transport">The inner transport.</param>
        /// <param name="provider">The SASL transport provider.</param>
        /// <param name="isInitiator">true if it is the transport initiator, otherwise false.</param>
        public SaslTransport(TransportBase transport, SaslTransportProvider provider, bool isInitiator)
            : base("sasl", transport.Identifier)
        {
            this.innerTransport = transport;
            this.negotiator = new SaslNegotiator(this, provider, isInitiator);
        }

        /// <summary>
        /// Gets the local endpoint.
        /// </summary>
        public override EndPoint LocalEndPoint
        {
            get
            {
                return this.innerTransport.LocalEndPoint;
            }
        }

        /// <summary>
        /// Gets the remote endpoint.
        /// </summary>
        public override EndPoint RemoteEndPoint
        {
            get
            {
                return this.innerTransport.RemoteEndPoint;
            }
        }

        /// <summary>
        /// true if the transport is encrypted, otherwise false.
        /// </summary>
        public override bool IsSecure
        {
            get { return this.innerTransport.IsSecure; }
        }

        /// <summary>
        /// Sets a transport monitor for transport I/O operations.
        /// </summary>
        /// <param name="usageMeter"></param>
        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.innerTransport.SetMonitor(usageMeter);
        }

        /// <summary>
        /// Starts a write operation.
        /// </summary>
        /// <param name="args">The write arguments.</param>
        /// <returns>true if the write operation is pending, otherwise false.</returns>
        public override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            return this.innerTransport.WriteAsync(args);
        }

        /// <summary>
        /// Starts a read operation.
        /// </summary>
        /// <param name="args">The read arguments.</param>
        /// <returns>true if the read operation is pending, otherwise false.</returns>
        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            return this.innerTransport.ReadAsync(args);
        }

        internal void OnNegotiationSucceed(IPrincipal principal)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "OnNegotiationSucceed");
            this.negotiator = null;
            this.Principal = principal;
            this.CompleteOpen(false, null);
        }

        internal void OnNegotiationFail(Exception exception)
        {
            AmqpTrace.Provider.AmqpLogError(this, "OnNegotiationFail", exception);
            this.negotiator = null;
            this.innerTransport.SafeClose(exception);
            this.CompleteOpen(false, exception);
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <returns>true if open is completed, otherwise false.</returns>
        protected override bool OpenInternal()
        {
            return this.negotiator.Start();
        }

        /// <summary>
        /// Aborts the object.
        /// </summary>
        protected override void AbortInternal()
        {
            this.innerTransport.Abort();
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>true if close is completed, otherwise false.</returns>
        protected override bool CloseInternal()
        {
            this.innerTransport.Close();
            return true;
        }
    }
}
