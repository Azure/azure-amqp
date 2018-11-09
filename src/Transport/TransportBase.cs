// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Security.Principal;

    public abstract class TransportBase : AmqpObject
    {
        protected TransportBase(string type)
            : base(type)
        {
        }

        protected TransportBase(string type, SequenceNumber identifier)
            : base(type, identifier)
        {
        }

        public abstract string LocalEndPoint
        {
            get; 
        }

        public abstract string RemoteEndPoint
        {
            get;
        }

        public IPrincipal Principal
        {
            get;
            protected set;
        }

        public virtual bool IsSecure
        {
            get { return false; }
        }

        public bool IsAuthenticated
        {
            get { return this.Principal != null && this.Principal.Identity.IsAuthenticated; }
        }

        public virtual bool RequiresCompleteFrames
        {
            get { return false; }
        }

        public abstract void SetMonitor(ITransportMonitor usageMeter);

        public abstract bool WriteAsync(TransportAsyncCallbackArgs args);

        public abstract bool ReadAsync(TransportAsyncCallbackArgs args);

        protected override void OnOpen(TimeSpan timeout)
        {
            this.State = AmqpObjectState.Opened;
        }

        protected override void OnClose(TimeSpan timeout)
        {
            this.CloseInternal();
            this.State = AmqpObjectState.End;
        }

        protected override bool OpenInternal()
        {
            return true;
        }
    }
}
