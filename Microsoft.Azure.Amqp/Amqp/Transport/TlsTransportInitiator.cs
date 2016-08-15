// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    
    /// <summary>
    /// This initiator establishes an SSL connection (no AMQP security upgrade)
    /// </summary>
    sealed class TlsTransportInitiator : TransportInitiator
    {
        static readonly AsyncCallback onTransportOpened = OnTransportOpened;

        TlsTransportSettings transportSettings;
        TransportAsyncCallbackArgs callbackArgs;
        TimeoutHelper timeoutHelper;

        public TlsTransportInitiator(TlsTransportSettings transportSettings)
        {
            this.transportSettings = transportSettings;
        }

        public override string ToString()
        {
            return "tls-initiator";
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            this.callbackArgs = callbackArgs;
            this.timeoutHelper = new TimeoutHelper(timeout);
            TransportInitiator innerInitiator = this.transportSettings.InnerTransportSettings.CreateInitiator();

            TransportAsyncCallbackArgs innerArgs = new TransportAsyncCallbackArgs();
            innerArgs.CompletedCallback = OnInnerTransportConnected;
            innerArgs.UserToken = this;
            if (innerInitiator.ConnectAsync(timeout, innerArgs))
            {
                // pending
                return true;
            }
            else
            {
                this.HandleInnerTransportConnected(innerArgs);
                return !this.callbackArgs.CompletedSynchronously;
            }
        }

        static void OnInnerTransportConnected(TransportAsyncCallbackArgs innerArgs)
        {
            TlsTransportInitiator thisPtr = (TlsTransportInitiator)innerArgs.UserToken;
            thisPtr.HandleInnerTransportConnected(innerArgs);
        }

        static void OnTransportOpened(IAsyncResult result)
        {
            if (result.CompletedSynchronously)
            {
                return;
            }

            TlsTransportInitiator thisPtr = (TlsTransportInitiator)result.AsyncState;
            try
            {
                thisPtr.HandleTransportOpened(result);
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                thisPtr.callbackArgs.Exception = exception;
            }

            thisPtr.Complete();
        }

        void HandleInnerTransportConnected(TransportAsyncCallbackArgs innerArgs)
        {
            this.callbackArgs.CompletedSynchronously = innerArgs.CompletedSynchronously;
            if (innerArgs.Exception != null)
            {
                this.callbackArgs.Exception = innerArgs.Exception;
                this.Complete();
            }
            else
            {
                Fx.Assert(innerArgs.Transport != null, "must have a valid inner transport");
                // upgrade transport
#if !PCL
                this.callbackArgs.Transport = new TlsTransport(innerArgs.Transport, this.transportSettings);
                try
                {
                    IAsyncResult result = this.callbackArgs.Transport.BeginOpen(this.timeoutHelper.RemainingTime(), onTransportOpened, this);
                    if (result.CompletedSynchronously)
                    {
                        this.HandleTransportOpened(result);
                        this.Complete();
                    }
                }
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    this.callbackArgs.Exception = exception;
                    this.Complete();
                }
#else
                throw new NotImplementedException();
#endif
            }
        }

        void HandleTransportOpened(IAsyncResult result)
        {
            this.callbackArgs.Transport.EndOpen(result);
            if (this.callbackArgs.CompletedSynchronously)
            {
                this.callbackArgs.CompletedSynchronously = result.CompletedSynchronously;
            }
        }

        void Complete()
        {
            if (this.callbackArgs.Exception != null && this.callbackArgs.Transport != null)
            {
                this.callbackArgs.Transport.SafeClose(this.callbackArgs.Exception);
                this.callbackArgs.Transport = null;
            }

            this.callbackArgs.CompletedCallback(this.callbackArgs);
        }
    }
}
