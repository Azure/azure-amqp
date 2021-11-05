// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    
    /// <summary>
    /// This listener accepts SSL transport directly (no AMQP security upgrade)
    /// </summary>
    public class TlsTransportListener : TransportListener
    {
        readonly AsyncCallback onTransportOpened;
        readonly TlsTransportSettings transportSettings;
        TransportListener innerListener;

        public TlsTransportListener(TlsTransportSettings transportSettings)
            : base("tls-listener")
        {
            this.transportSettings = transportSettings;
            this.onTransportOpened = this.OnTransportOpened;
        }

        protected override bool CloseInternal()
        {
            if (this.innerListener != null)
            {
                this.innerListener.Close();
            }

            return true;
        }

        protected override void AbortInternal()
        {
            if (this.innerListener != null)
            {
                this.innerListener.Abort();
            }
        }

        protected override void OnListen()
        {
            TransportSettings innerSettings = this.transportSettings.InnerTransportSettings;
            this.innerListener = innerSettings.CreateListener();
            this.innerListener.Closed += new EventHandler(OnInnerListenerClosed);
            this.innerListener.Listen(this.OnAcceptInnerTransport);
        }

        protected virtual TlsTransport OnCreateTransport(TransportBase innerTransport, TlsTransportSettings tlsTransportSettings)
        {
            return new TlsTransport(innerTransport, tlsTransportSettings);
        }

        void OnInnerListenerClosed(object sender, EventArgs e)
        {
            if (!this.IsClosing())
            {
                TransportListener innerListener = (TransportListener)sender;
                this.SafeClose(innerListener.TerminalException);
            }
        }

        void OnAcceptInnerTransport(TransportListener innerListener, TransportAsyncCallbackArgs innerArgs)
        {
            Fx.Assert(innerArgs.Exception == null, "Should not be called with an exception.");
            Fx.Assert(innerArgs.Transport != null, "Should be called with a transport.");
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Accept, innerArgs.Transport);

            try
            {
                // upgrade transport
                innerArgs.Transport = this.OnCreateTransport(innerArgs.Transport, this.transportSettings);
                IAsyncResult result = innerArgs.Transport.BeginOpen(
                    AmqpConstants.DefaultTimeout, 
                    this.onTransportOpened,
                    innerArgs);
                if (result.CompletedSynchronously)
                {
                    this.HandleTransportOpened(result);
                    return;
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                innerArgs.Transport.SafeClose(exception);
            }
        }

        void OnTransportOpened(IAsyncResult result)
        {
            if (result.CompletedSynchronously)
            {
                return;
            }

            try
            {
                this.HandleTransportOpened(result);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                TransportAsyncCallbackArgs innerArgs = (TransportAsyncCallbackArgs)result.AsyncState;
                innerArgs.Transport.SafeClose(exception);
            }
        }

        void HandleTransportOpened(IAsyncResult result)
        {
            TransportAsyncCallbackArgs innerArgs = (TransportAsyncCallbackArgs)result.AsyncState;
            innerArgs.Transport.EndOpen(result);
            if (innerArgs.CompletedSynchronously)
            {
                innerArgs.CompletedSynchronously = result.CompletedSynchronously;
            }

            this.OnTransportAccepted(innerArgs);
        }
    }
}
