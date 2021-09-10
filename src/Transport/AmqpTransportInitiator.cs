// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// The initiator creates an AMQP transport.
    /// </summary>
    /// <remarks>Transport upgrades are supported by providers in the protocol settings.
    /// The initiator establishes a base transport using the transport settings,
    /// and then it iterates through the transport provider list to upgrade
    /// the transports (e.g. tcp -> tls -> sasl -> amqp).
    /// </remarks>
    public sealed class AmqpTransportInitiator : TransportInitiator
    {
        AmqpSettings settings;
        TransportSettings transportSettings;
        AsyncIO.AsyncBufferWriter writer;
        AsyncIO.AsyncBufferReader reader;
        TimeoutHelper timeoutHelper;
        int providerIndex;
        ProtocolHeader sentHeader;
        int completingThread;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="settings">The protocol settings.</param>
        /// <param name="transportSettings">The base transport settings.</param>
        public AmqpTransportInitiator(AmqpSettings settings, TransportSettings transportSettings)
        {
            settings.ValidateInitiatorSettings();
            this.settings = settings;
            this.transportSettings = transportSettings;
        }

        static int CurrentThreadId
        {
            get
            {
                return Thread.CurrentThread.ManagedThreadId;
            }
        }

        /// <summary>
        /// Connects to the remote peer.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callbackArgs">The callback arguments.</param>
        /// <returns>true if connect is pending, false otherwise.</returns>
        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Connect, this.transportSettings);
            TransportInitiator innerInitiator = this.transportSettings.CreateInitiator();
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.UserToken = this;
            args.CompletedCallback = _args => ((AmqpTransportInitiator)_args.UserToken).OnConnectComplete(_args);
            args.UserToken2 = callbackArgs;
            callbackArgs.CompletedSynchronously = false;
            this.timeoutHelper = new TimeoutHelper(timeout);
            if (innerInitiator.ConnectAsync(timeout, args))
            {
                return true;
            }

            int currentThread = CurrentThreadId;
            Interlocked.Exchange(ref this.completingThread, currentThread);
            this.OnConnectComplete(args);
            return Interlocked.Exchange(ref this.completingThread, -1) != 0;
        }

        /// <summary>
        /// Begins to connect to the remote peer.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The object associated with the operation.</param>
        /// <returns>An <see cref="IAsyncResult"/>.</returns>
        public IAsyncResult BeginConnect(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return new ConnectAsyncResult(this, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Ends the connect operation.
        /// </summary>
        /// <param name="result">The <see cref="IAsyncResult"/> returned by the begin method.</param>
        /// <returns>An AMQP transport.</returns>
        public TransportBase EndConnect(IAsyncResult result)
        {
            return ConnectAsyncResult.End(result);
        }

        /// <summary>
        /// Gets a string representation of the object.
        /// </summary>
        /// <returns>A string representation of the object.</returns>
        public override string ToString()
        {
            return "tp-initiator";
        }

        /// <summary>
        /// Starts the connect operation.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task that returns a transport when it is completed.</returns>
        [Obsolete]
        public Task<TransportBase> ConnectTaskAsync(TimeSpan timeout)
        {
            return this.ConnectAsync(timeout);
        }

        /// <summary>
        /// Starts the connect operation.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task that returns a transport when it is completed.</returns>
        public Task<TransportBase> ConnectAsync(TimeSpan timeout)
        {
            return this.ConnectAsync(timeout, CancellationToken.None);
        }

        /// <summary>
        /// Starts the connect operation.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A task that returns a transport when it is completed.</returns>
        public Task<TransportBase> ConnectAsync(CancellationToken cancellationToken)
        {
            return this.ConnectAsync(TimeSpan.MaxValue, cancellationToken);
        }

        internal Task<TransportBase> ConnectAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (thisPtr, t, k, c, s) => new ConnectAsyncResult(thisPtr, t, k, c, s),
                static r => ConnectAsyncResult.End(r),
                this,
                timeout,
                cancellationToken,
                null);
        }

        void OnConnectComplete(TransportAsyncCallbackArgs args)
        {
            if (args.Exception != null)
            {
                this.Complete(args);
                return;
            }

            TransportProvider provider = this.settings.TransportProviders[this.providerIndex];
            if (provider.ProtocolId == ProtocolId.Amqp)
            {
                this.Complete(args);
                return;
            }

            this.writer = new AsyncIO.AsyncBufferWriter(args.Transport);
            this.reader = new AsyncIO.AsyncBufferReader(args.Transport);
            this.WriteSecurityHeader(args);
        }

        void WriteSecurityHeader(TransportAsyncCallbackArgs args)
        {
            // secure transport: header negotiation
            TransportProvider provider = this.settings.TransportProviders[this.providerIndex];
            this.sentHeader = new ProtocolHeader(provider.ProtocolId, provider.DefaultVersion);
            AmqpTrace.OnProtocolHeader(this.sentHeader, true);

            ByteBuffer buffer = new ByteBuffer(new byte[AmqpConstants.ProtocolHeaderSize]);
            this.sentHeader.Encode(buffer);

            args.SetBuffer(buffer.Buffer, buffer.Offset, buffer.Length);
            args.CompletedCallback = this.OnWriteHeaderComplete;
            this.writer.WriteBuffer(args);
        }

        void OnWriteHeaderComplete(TransportAsyncCallbackArgs args)
        {
            if (args.Exception != null)
            {
                this.Complete(args);
                return;
            }

            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "ReadHeader");
            byte[] headerBuffer = new byte[AmqpConstants.ProtocolHeaderSize];
            args.SetBuffer(headerBuffer, 0, headerBuffer.Length);
            args.CompletedCallback = this.OnReadHeaderComplete;
            this.reader.ReadBuffer(args);
        }

        void OnReadHeaderComplete(TransportAsyncCallbackArgs args)
        {
            if (args.Exception != null)
            {
                AmqpTrace.Provider.AmqpLogError(this, "ReadHeader", args.Exception);
                this.Complete(args);
                return;
            }

            try
            {
                ProtocolHeader receivedHeader = new ProtocolHeader();
                receivedHeader.Decode(new ByteBuffer(args.Buffer, args.Offset, args.Count));
                AmqpTrace.OnProtocolHeader(receivedHeader, false);

                if (!receivedHeader.Equals(this.sentHeader))
                {
                    // TODO: need to reconnect with the reply version if supported
                    throw new AmqpException(AmqpErrorCode.NotImplemented, AmqpResources.GetString(AmqpResources.AmqpProtocolVersionNotSupported, this.sentHeader, receivedHeader));
                }

                // upgrade transport
                TransportBase secureTransport = this.settings.TransportProviders[this.providerIndex].CreateTransport(args.Transport, true);
                AmqpTrace.Provider.AmqpUpgradeTransport(this, args.Transport, secureTransport);
                args.Transport = secureTransport;
                IAsyncResult result = args.Transport.BeginOpen(this.timeoutHelper.RemainingTime(), this.OnTransportOpenCompete, args);
                if (result.CompletedSynchronously)
                {
                    this.HandleTransportOpened(result);
                }
            }
            catch (Exception exp) when (!Fx.IsFatal(exp))
            {
                AmqpTrace.Provider.AmqpLogError(this, "OnProtocolHeader", exp);
                args.Exception = exp;
                this.Complete(args);
            }
        }

        void OnTransportOpenCompete(IAsyncResult result)
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
                TransportAsyncCallbackArgs args = (TransportAsyncCallbackArgs)result.AsyncState;
                args.Exception = exception;
                this.Complete(args);
            }
        }

        void HandleTransportOpened(IAsyncResult result)
        {
            TransportAsyncCallbackArgs args = (TransportAsyncCallbackArgs)result.AsyncState;
            args.Transport.EndOpen(result);

            ++this.providerIndex;
            if (this.providerIndex == this.settings.TransportProviders.Count ||
                this.settings.TransportProviders[this.providerIndex].ProtocolId == ProtocolId.Amqp)
            {
                this.writer = null;
                this.reader = null;
                this.providerIndex = 0;
                this.Complete(args);
            }
            else
            {
                this.writer = new AsyncIO.AsyncBufferWriter(args.Transport);
                this.reader = new AsyncIO.AsyncBufferReader(args.Transport);
                this.WriteSecurityHeader(args);
            }
        }

        void Complete(TransportAsyncCallbackArgs args)
        {
            if (args.Exception != null && args.Transport != null)
            {
                args.Transport.SafeClose(args.Exception);
                args.Transport = null;
            }

            TransportAsyncCallbackArgs innerArgs = (TransportAsyncCallbackArgs)args.UserToken2;
            innerArgs.Transport = args.Transport;
            innerArgs.Exception = args.Exception;

            int currentThread = CurrentThreadId;
            innerArgs.CompletedSynchronously = Interlocked.Add(ref this.completingThread, -currentThread) == 0;
            if (!innerArgs.CompletedSynchronously)
            {
                innerArgs.CompletedCallback(innerArgs);
            }
        }

        sealed class ConnectAsyncResult : TimeoutAsyncResult<string>
        {
            // Inner initiator may also have a timer, so don't compete with them.
            const int BufferedTimeInTicks = 5000 * 10000;
            static readonly Action<TransportAsyncCallbackArgs> onConnect = OnConnect;
            readonly AmqpTransportInitiator initiator;
            readonly TransportAsyncCallbackArgs args;

            public ConnectAsyncResult(AmqpTransportInitiator initiator,
                TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
                : base(GetBufferedTimeout(timeout), cancellationToken, callback, state)
            {
                this.initiator = initiator;
                this.args = new TransportAsyncCallbackArgs();
                this.args.CompletedCallback = onConnect;
                this.args.UserToken = this;
                if (!initiator.ConnectAsync(timeout, this.args))
                {
                    OnConnect(this.args);
                }

                this.StartTracking();
            }

            protected override string Target
            {
                get
                {
                    return this.initiator.transportSettings.ToString();
                }
            }

            public static TransportBase End(IAsyncResult result)
            {
                return AsyncResult.End<ConnectAsyncResult>(result).args.Transport;
            }

            public override void Cancel()
            {
                this.args.Transport?.Abort();
                this.CompleteSelf(false, new TaskCanceledException());
            }

            protected override void CompleteOnTimer()
            {
                if (this.args.Transport != null)
                {
                    this.args.Transport.Abort();
                }

                base.CompleteOnTimer();
            }

            static TimeSpan GetBufferedTimeout(TimeSpan timeout)
            {
                if (timeout.Ticks < TimeSpan.MaxValue.Ticks - BufferedTimeInTicks)
                {
                    return TimeSpan.FromTicks(timeout.Ticks + BufferedTimeInTicks);
                }

                return TimeSpan.MaxValue;
            }

            static void OnConnect(TransportAsyncCallbackArgs args)
            {
                ConnectAsyncResult thisPtr = (ConnectAsyncResult)args.UserToken;
                if (!thisPtr.CompleteSelf(args.CompletedSynchronously, args.Exception))
                {
                    if (args.Transport != null)
                    {
                        // completed by timer
                        args.Transport.Abort();
                    }
                }
            }
        }
    }
}
