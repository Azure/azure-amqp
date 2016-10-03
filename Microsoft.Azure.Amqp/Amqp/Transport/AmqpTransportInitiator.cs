// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Tracing;

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
        /// This initiator establishes a base transport using the transport settings
        /// Then it iterates through the security provider list in the settings to upgrade
        /// the transport (e.g. tcp -> tls -> sasl).
        /// </summary>
        /// <param name="settings"></param>
        /// <param name="transportSettings"></param>
        public AmqpTransportInitiator(AmqpSettings settings, TransportSettings transportSettings)
        {
            settings.ValidateInitiatorSettings();
            this.settings = settings;
            this.transportSettings = transportSettings;
        }

        public override bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Connect, this.transportSettings);
            TransportInitiator innerInitiator = this.transportSettings.CreateInitiator();
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.CompletedCallback = this.OnConnectComplete;
            args.UserToken = callbackArgs;
            callbackArgs.CompletedSynchronously = false;
            this.timeoutHelper = new TimeoutHelper(timeout);
            if (innerInitiator.ConnectAsync(timeout, args))
            {
                return true;
            }

            int currentThread = Thread.CurrentThread.ManagedThreadId;
            Interlocked.Exchange(ref this.completingThread, currentThread);
            this.OnConnectComplete(args);
            return Interlocked.Exchange(ref this.completingThread, -1) != 0;
        }

        public IAsyncResult BeginConnect(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return new ConnectAsyncResult(this, timeout, callback, state);
        }

        public TransportBase EndConnect(IAsyncResult result)
        {
            return ConnectAsyncResult.End(result);
        }

        public override string ToString()
        {
            return "tp-initiator";
        }

        public Task<TransportBase> ConnectTaskAsync(TimeSpan timeout)
        {
            var tcs = new TaskCompletionSource<TransportBase>();
            var args = new TransportAsyncCallbackArgs
            {
                CompletedCallback = a =>
                {
                    if (a.Exception != null)
                    {
                        tcs.SetException(a.Exception);
                    }
                    else
                    {
                        tcs.SetResult(a.Transport);
                    }
                }
            };

            if (!this.ConnectAsync(timeout, args))
            {
                args.CompletedCallback(args);
            }

            return tcs.Task;
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
#if DEBUG
            this.sentHeader.Trace(true);
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Send, this.sentHeader);
#endif

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
                AmqpTrace.Provider.AmqpLogError(this, "ReadHeader", args.Exception.Message);
                this.Complete(args);
                return;
            }

            try
            {
                ProtocolHeader receivedHeader = new ProtocolHeader();
                receivedHeader.Decode(new ByteBuffer(args.Buffer, args.Offset, args.Count));
#if DEBUG
                receivedHeader.Trace(false);
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Receive, receivedHeader);
#endif

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
            catch (Exception exp)
            {
                if (Fx.IsFatal(exp))
                {
                    throw;
                }

                AmqpTrace.Provider.AmqpLogError(this, "OnProtocolHeader", exp.Message);
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
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

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

            TransportAsyncCallbackArgs innerArgs = (TransportAsyncCallbackArgs)args.UserToken;
            innerArgs.Transport = args.Transport;
            innerArgs.Exception = args.Exception;

            int currentThread = Thread.CurrentThread.ManagedThreadId;
            innerArgs.CompletedSynchronously = Interlocked.Add(ref this.completingThread, -currentThread) == 0;
            if (!innerArgs.CompletedSynchronously)
            {
                innerArgs.CompletedCallback(innerArgs);
            }
        }

        sealed class ConnectAsyncResult : AsyncResult
        {
            static Action<TransportAsyncCallbackArgs> onConnect = OnConnect;
            TransportBase transport;

            public ConnectAsyncResult(AmqpTransportInitiator initiator, TimeSpan timeout, AsyncCallback callback, object state)
                : base(callback, state)
            {
                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.CompletedCallback = onConnect;
                args.UserToken = this;
                if (!initiator.ConnectAsync(timeout, args))
                {
                    OnConnect(args);
                }
            }

            public static TransportBase End(IAsyncResult result)
            {
                return AsyncResult.End<ConnectAsyncResult>(result).transport;
            }

            static void OnConnect(TransportAsyncCallbackArgs args)
            {
                ConnectAsyncResult thisPtr = (ConnectAsyncResult)args.UserToken;
                if (args.Exception != null)
                {
                    thisPtr.Complete(args.CompletedSynchronously, args.Exception);
                }
                else
                {
                    thisPtr.transport = args.Transport;
                    thisPtr.Complete(args.CompletedSynchronously);
                }
            }
        }
    }
}
