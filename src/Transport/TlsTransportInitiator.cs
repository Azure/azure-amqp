// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    
    /// <summary>
    /// This initiator creates a TLS transport directly. (no TLS upgrade)
    /// </summary>
    public class TlsTransportInitiator : TransportInitiator
    {
        static readonly AsyncCallback onTransportOpened = OnTransportOpened;

        readonly TlsTransportSettings transportSettings;
        TransportAsyncCallbackArgs callbackArgs;
        TimeoutHelper timeoutHelper;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="transportSettings">The TLS transport settings.</param>
        public TlsTransportInitiator(TlsTransportSettings transportSettings)
        {
            this.transportSettings = transportSettings;
        }

        /// <summary>
        /// Gets a string representation of the object.
        /// </summary>
        /// <returns>A string representation of the object.</returns>
        public override string ToString()
        {
            return "tls-initiator";
        }

        /// <summary>
        /// Connects to the remote peer.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callbackArgs">The callback arguments.</param>
        /// <returns>true if the operation is pending, false otherwise.</returns>
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

        /// <summary>
        /// Creates a TLS transport from the inner transport.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="tlsTransportSettings">The TLS transport settings.</param>
        /// <returns></returns>
        protected virtual TlsTransport OnCreateTransport(TransportBase innerTransport, TlsTransportSettings tlsTransportSettings)
        {
            return new TlsTransport(innerTransport, tlsTransportSettings);
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
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
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
                this.callbackArgs.Transport = this.OnCreateTransport(innerArgs.Transport, this.transportSettings);
                try
                {
                    IAsyncResult result = this.callbackArgs.Transport.BeginOpen(this.timeoutHelper.RemainingTime(), onTransportOpened, this);
                    if (result.CompletedSynchronously)
                    {
                        this.HandleTransportOpened(result);
                        this.Complete();
                    }
                    else
                    {
                        this.callbackArgs.CompletedSynchronously = false;
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    this.callbackArgs.Exception = exception;
                    this.Complete();
                }
            }
        }

        void HandleTransportOpened(IAsyncResult result)
        {
            this.callbackArgs.Transport.EndOpen(result);
        }

        void Complete()
        {
            if (this.callbackArgs.Exception != null && this.callbackArgs.Transport != null)
            {
                this.callbackArgs.Transport.SafeClose(this.callbackArgs.Exception);
                this.callbackArgs.Transport = null;
            }

            if (!this.callbackArgs.CompletedSynchronously)
            {
                this.callbackArgs.CompletedCallback(this.callbackArgs);
            }
        }
    }
}
