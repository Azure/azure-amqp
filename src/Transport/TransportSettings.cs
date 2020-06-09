// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    /// <summary>
    /// Defines the transport settings.
    /// </summary>
    public abstract class TransportSettings
    {
        int sendBufferSize;
        int receiveBufferSize;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        protected TransportSettings()
        {
            this.sendBufferSize = -1;
            this.receiveBufferSize = -1;
        }

        /// <summary>
        /// Gets or sets the concurrent tasks in a transport listener
        /// to accept incoming transports.
        /// </summary>
        public int ListenerAcceptorCount
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the send buffer size.
        /// </summary>
        public int SendBufferSize
        {
            get { return this.sendBufferSize >= 0 ? this.sendBufferSize : AmqpConstants.TransportBufferSize; }
            set { this.sendBufferSize = value; }
        }

        /// <summary>
        /// Gets or sets the receive buffer size.
        /// </summary>
        public int ReceiveBufferSize
        {
            get { return this.receiveBufferSize >= 0 ? this.receiveBufferSize : AmqpConstants.TransportBufferSize; }
            set { this.receiveBufferSize = value; }
        }

        internal int InternalSendBufferSize
        {
            get { return this.sendBufferSize; }
        }

        internal int InternalReceiveBufferSize
        {
            get { return this.receiveBufferSize; }
        }

        /// <summary>
        /// Creates a transport initiator.
        /// </summary>
        /// <returns>The transport initiator.</returns>
        public abstract TransportInitiator CreateInitiator();

        /// <summary>
        /// Creates a transport listener.
        /// </summary>
        /// <returns>The transport listener.</returns>
        public abstract TransportListener CreateListener();
    }
}
