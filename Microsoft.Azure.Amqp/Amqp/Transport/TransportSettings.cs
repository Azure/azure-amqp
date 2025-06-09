// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    public abstract class TransportSettings
    {
        int sendBufferSize;
        int receiveBufferSize;

        protected TransportSettings()
        {
            this.sendBufferSize = -1;
            this.receiveBufferSize = -1;
        }

        public int ListenerAcceptorCount
        {
            get;
            set;
        }

        public int SendBufferSize
        {
            get { return this.sendBufferSize >= 0 ? this.sendBufferSize : AmqpConstants.TransportBufferSize; }
            set { this.sendBufferSize = value; }
        }

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

        public abstract TransportInitiator CreateInitiator();

#if NET45 || NETSTANDARD || MONOANDROID || NET6_0_OR_GREATER
        public abstract TransportListener CreateListener();
#endif
    }
}
