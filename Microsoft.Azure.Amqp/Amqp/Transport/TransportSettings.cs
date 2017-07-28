// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    public abstract class TransportSettings
    {
        protected TransportSettings()
        {
            this.SendBufferSize = AmqpConstants.TransportBufferSize;
            this.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
        }

        public int ListenerAcceptorCount
        {
            get;
            set;
        }

        public int SendBufferSize
        {
            get;
            set;
        }

        public int ReceiveBufferSize
        {
            get;
            set;
        }

        public abstract TransportInitiator CreateInitiator();

#if NET45 || NETSTANDARD || MONOANDROID
        public abstract TransportListener CreateListener();
#endif
    }
}
