// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Security.Principal;

    /// <summary>
    /// Defines the base class of all transports.
    /// </summary>
    public abstract class TransportBase : AmqpObject
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="type">A prefix to the object name for debugging purposes.</param>
        protected TransportBase(string type)
            : base(type)
        {
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="type">A prefix to the object name for debugging purposes.</param>
        /// <param name="identifier">The identifier of the object.</param>
        protected TransportBase(string type, SequenceNumber identifier)
            : base(type, identifier)
        {
        }

        /// <summary>
        /// Gets the local endpoint.
        /// </summary>
        internal abstract EndPoint Local
        {
            get; 
        }

        /// <summary>
        /// Gets the remote endpoint.
        /// </summary>
        internal abstract EndPoint Remote
        {
            get;
        }

        /// <summary>
        /// Gets the local endpoint string.
        /// </summary>
        public abstract string LocalEndPoint
        {
            get;
        }

        /// <summary>
        /// Gets the remote endpoint string.
        /// </summary>
        public abstract string RemoteEndPoint
        {
            get;
        }

        /// <summary>
        /// Gets or sets the identity information.
        /// </summary>
        public IPrincipal Principal
        {
            get;
            protected set;
        }

        /// <summary>
        /// true if the transport is encrypted, otherwise false.
        /// </summary>
        public virtual bool IsSecure
        {
            get { return false; }
        }

        /// <summary>
        /// true if the transport is authenticated, otherwise false.
        /// </summary>
        public bool IsAuthenticated
        {
            get { return this.Principal != null && this.Principal.Identity.IsAuthenticated; }
        }

        /// <summary>
        /// Gets a value indicating whether the transport requires complete frames.
        /// </summary>
        public virtual bool RequiresCompleteFrames
        {
            get { return false; }
        }

        /// <summary>
        /// Sets a transport monitor for transport I/O operations.
        /// </summary>
        /// <param name="usageMeter">The transport monitor.</param>
        public abstract void SetMonitor(ITransportMonitor usageMeter);

        /// <summary>
        /// Starts a write operation.
        /// </summary>
        /// <param name="args">The write arguments.</param>
        /// <returns>true if the write operation is pending, otherwise false.</returns>
        public abstract bool WriteAsync(TransportAsyncCallbackArgs args);

        /// <summary>
        /// Starts a read operation.
        /// </summary>
        /// <param name="args">The read arguments.</param>
        /// <returns>true if the read operation is pending, otherwise false.</returns>
        public abstract bool ReadAsync(TransportAsyncCallbackArgs args);

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        protected override void OnOpen(TimeSpan timeout)
        {
            this.State = AmqpObjectState.Opened;
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        protected override void OnClose(TimeSpan timeout)
        {
            this.CloseInternal();
            this.State = AmqpObjectState.End;
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <returns>true if open is completed, otherwise false.</returns>
        protected override bool OpenInternal()
        {
            return true;
        }
    }
}
