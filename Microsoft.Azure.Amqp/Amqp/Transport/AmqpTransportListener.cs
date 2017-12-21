// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// This listener supports protocol upgrade (e.g. tcp -> tls -> sasl)
    /// </summary>
    public sealed class AmqpTransportListener : TransportListener
    {
        readonly List<TransportListener> innerListeners;
        readonly AmqpSettings settings;

        public AmqpTransportListener(IEnumerable<TransportListener> listeners, AmqpSettings settings)
            : base("tp-listener")
        {
            this.innerListeners = new List<TransportListener>(listeners);
            this.settings = settings;
        }

        public AmqpSettings AmqpSettings
        {
            get { return this.settings; }
        }

        public T Find<T>() where T : TransportListener
        {
            foreach (TransportListener listener in this.innerListeners)
            {
                if (typeof(T) == listener.GetType())
                {
                    return (T)listener;
                }
            }

            return null;
        }

        protected override void OnListen()
        {
            Action<TransportListener, TransportAsyncCallbackArgs> onTransportAccept = this.OnAcceptTransport;
            EventHandler onListenerClose = this.OnListenerClosed;
            foreach (TransportListener listener in this.innerListeners)
            {
                listener.Closed += onListenerClose;
                listener.Listen(onTransportAccept);
            }
        }

        protected override bool CloseInternal()
        {
            this.State = AmqpObjectState.CloseSent;
            foreach (TransportListener listener in this.innerListeners.ToArray())
            {
                listener.Close();
            }

            return true;
        }

        protected override void AbortInternal()
        {
            this.State = AmqpObjectState.Faulted;
            foreach (TransportListener listener in this.innerListeners.ToArray())
            {
                listener.Abort();
            }
        }

        void OnListenerClosed(object sender, EventArgs e)
        {
            if (!this.IsClosing())
            {
                // If we weren't shutting down then this class needs to Close itself (with the TerminalException) since
                // it is no longer doing all the listening it is supposed to do.
                TransportListener innerListener = (TransportListener)sender;
                AmqpTrace.Provider.AmqpLogError(this, "OnListenerClosed", innerListener.ToString());
                this.SafeClose(innerListener.TerminalException);
            }
        }

        void OnAcceptTransport(TransportListener innerListener, TransportAsyncCallbackArgs args)
        {
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, "OnAcceptTransport");
            TransportHandler.SpawnHandler(this, args);
        }

        void OnHandleTransportComplete(TransportAsyncCallbackArgs args)
        {
            args.SetBuffer(null, 0, 0);
            args.CompletedCallback = null;

            if (args.Exception != null)
            {
                args.Transport.SafeClose(args.Exception);
            }
            else
            {
                this.OnTransportAccepted(args);
            }
        }

        sealed class TransportHandler
        {
            readonly static AsyncCallback onTransportOpened = OnTransportOpened;

            readonly AmqpTransportListener parent;
            readonly TransportAsyncCallbackArgs args;
            static Action<TransportAsyncCallbackArgs> readCompleteCallback = OnReadHeaderComplete;
            static Action<TransportAsyncCallbackArgs> writeCompleteCallback = OnWriteHeaderComplete;
            AsyncIO.AsyncBufferReader bufferReader;
            AsyncIO.AsyncBufferWriter bufferWriter;
            byte[] buffer;
            TimeoutHelper timeoutHelper;

            TransportHandler(AmqpTransportListener parent, TransportAsyncCallbackArgs args)
            {
                this.parent = parent;
                this.args = args;
                this.args.UserToken = this;
                this.buffer = new byte[AmqpConstants.ProtocolHeaderSize];
                this.bufferReader = new AsyncIO.AsyncBufferReader(args.Transport);
                this.bufferWriter = new AsyncIO.AsyncBufferWriter(args.Transport);
                this.timeoutHelper = new TimeoutHelper(AmqpConstants.DefaultTimeout);
            }

            public static void SpawnHandler(AmqpTransportListener parent, TransportAsyncCallbackArgs args)
            {
                TransportHandler handler = new TransportHandler(parent, args);
                ActionItem.Schedule(s => Start(s), handler);
            }

            public override string ToString()
            {
                return "tp-handler";
            }

            static void Start(object state)
            {
                TransportHandler thisPtr = (TransportHandler)state;
                thisPtr.ReadProtocolHeader();
            }

            static void OnTransportOpened(IAsyncResult result)
            {
                if (result.CompletedSynchronously)
                {
                    return;
                }

                TransportHandler thisPtr = (TransportHandler)result.AsyncState;
                try
                {
                    thisPtr.HandleTransportOpened(result);
                }
                catch (Exception exp) when (!Fx.IsFatal(exp))
                {
                    AmqpTrace.Provider.AmqpLogError(thisPtr, "HandleTransportOpened", exp.Message);
                    thisPtr.args.Exception = exp;
                    thisPtr.parent.OnHandleTransportComplete(thisPtr.args);
                }
            }

            void ReadProtocolHeader()
            {
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, "ReadHeader");
                this.args.SetBuffer(this.buffer, 0, this.buffer.Length);
                this.args.CompletedCallback = TransportHandler.readCompleteCallback;
                this.bufferReader.ReadBuffer(this.args);
            }

            static void OnReadHeaderComplete(TransportAsyncCallbackArgs args)
            {
                TransportHandler thisPtr = (TransportHandler)args.UserToken;
                if (args.Exception != null)
                {
                    thisPtr.parent.OnHandleTransportComplete(args);
                    return;
                }

                ByteBuffer buffer = new ByteBuffer(thisPtr.buffer, 0, thisPtr.buffer.Length);
                try
                {
                    thisPtr.OnProtocolHeader(buffer);
                }
                catch (Exception exp) when (!Fx.IsFatal(exp))
                {
                    AmqpTrace.Provider.AmqpLogError(thisPtr, "OnProtocolHeader", exp.Message);
                    args.Exception = exp;
                    thisPtr.parent.OnHandleTransportComplete(args);
                }
            }

            void OnProtocolHeader(ByteBuffer buffer)
            {
                ProtocolHeader header = new ProtocolHeader();
                header.Decode(buffer);
                AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Receive, header);

                // Protocol id negotiation
                TransportProvider provider = null;
                if (!this.parent.settings.TryGetTransportProvider(header, out provider))
                {
                    Fx.Assert(provider != null, "At least on provider should be configured.");
                    this.WriteReplyHeader(new ProtocolHeader(provider.ProtocolId, provider.DefaultVersion), true);
                    return;
                }

                // Protocol version negotiation
                AmqpVersion version;
                if (!provider.TryGetVersion(header.Version, out version))
                {
                    this.WriteReplyHeader(new ProtocolHeader(provider.ProtocolId, version), true);
                    return;
                }

                TransportBase newTransport = null;
                try
                {
                    newTransport = provider.CreateTransport(this.args.Transport, false);
                }
                catch (InvalidOperationException ioe)
                {
                    // treat this the same as protocol ID/version failure
                    // which are all client config issues
                    AmqpTrace.Provider.AmqpLogError(this, "CreateTransport", ioe.Message);
                    this.WriteReplyHeader(ProtocolHeader.Amqp100, true);
                    return;
                }

                if (object.ReferenceEquals(newTransport, this.args.Transport))
                {
                    if ((this.parent.settings.RequireSecureTransport && !newTransport.IsSecure) ||
                        (!this.parent.settings.AllowAnonymousConnection && !newTransport.IsAuthenticated))
                    {
                        AmqpTrace.Provider.AmqpInsecureTransport(this.parent, newTransport, newTransport.IsSecure, newTransport.IsAuthenticated);
                        this.WriteReplyHeader(this.parent.settings.GetDefaultHeader(), true);
                    }
                    else
                    {
                        this.args.UserToken = header;
                        this.parent.OnHandleTransportComplete(this.args);
                    }
                }
                else
                {
#if DEBUG
                    header.Trace(false);
#endif
                    AmqpTrace.Provider.AmqpUpgradeTransport(this, args.Transport, newTransport);
                    this.args.Transport = newTransport;
                    this.WriteReplyHeader(header, false);
                }
            }

            void HandleTransportOpened(IAsyncResult result)
            {
                this.args.Transport.EndOpen(result);
                this.bufferReader = new AsyncIO.AsyncBufferReader(this.args.Transport);
                this.bufferWriter = new AsyncIO.AsyncBufferWriter(this.args.Transport);
                this.ReadProtocolHeader();
            }

            void WriteReplyHeader(ProtocolHeader header, bool fail)
            {
                AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Send, header);
#if DEBUG
                header.Trace(true);
#endif

                ByteBuffer byteBuffer = new ByteBuffer(this.buffer);
                header.Encode(byteBuffer);
                this.args.SetBuffer(this.buffer, 0, this.buffer.Length);
                this.args.CompletedCallback = fail ? null : TransportHandler.writeCompleteCallback;
                this.bufferWriter.WriteBuffer(this.args);

                if (fail)
                {
                    this.args.Exception = new NotSupportedException(header.ToString());
                    this.parent.OnHandleTransportComplete(this.args);
                }
            }

            static void OnWriteHeaderComplete(TransportAsyncCallbackArgs args)
            {
                TransportHandler thisPtr = (TransportHandler)args.UserToken;
                if (args.Exception != null)
                {
                    thisPtr.parent.OnHandleTransportComplete(args);
                    return;
                }

                try
                {
                    IAsyncResult result = thisPtr.args.Transport.BeginOpen(thisPtr.timeoutHelper.RemainingTime(), onTransportOpened, thisPtr);
                    if (result.CompletedSynchronously)
                    {
                        thisPtr.HandleTransportOpened(result);
                    }
                }
                catch (Exception exp)
                {
                    args.Exception = exp;
                    thisPtr.parent.OnHandleTransportComplete(args);
                }
            }
        }
    }
}
