// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net.Security;
    using System.Security.Authentication;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.X509;

    public class TlsTransport : TransportBase, IDisposable
    {
        static readonly AsyncCallback onOpenComplete = OnOpenComplete;
        readonly TransportBase innerTransport;
        protected readonly CustomSslStream sslStream;
        TlsTransportSettings tlsSettings;
        OperationState writeState;
        OperationState readState;

        public TlsTransport(TransportBase innerTransport, TlsTransportSettings tlsSettings)
            : base("tls", innerTransport.Identifier)
        {
            Fx.Assert((tlsSettings.IsInitiator && tlsSettings.TargetHost != null) || (!tlsSettings.IsInitiator && tlsSettings.Certificate != null),
                tlsSettings.IsInitiator ? "Must have a target host for the client." : "Must have a certificate for the server.");
            this.innerTransport = innerTransport;
            this.tlsSettings = tlsSettings;
            this.sslStream = tlsSettings.CertificateValidationCallback == null ?
                new CustomSslStream(new TransportStream(this.innerTransport), false, tlsSettings.IsInitiator) :
                new CustomSslStream(new TransportStream(this.innerTransport), false, this.ValidateRemoteCertificate, tlsSettings.IsInitiator);
        }

        public override string LocalEndPoint => this.innerTransport.LocalEndPoint;

        public override string RemoteEndPoint => this.innerTransport.RemoteEndPoint;

        public override bool IsSecure => true;

        protected TlsTransportSettings TlsSettings
        {
            get { return this.tlsSettings; }
        }


        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.innerTransport.SetMonitor(usageMeter);
        }

        public override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(this.writeState.Args == null, "Cannot write when a write is still in progress");
            ArraySegment<byte> buffer;
            if (args.Buffer != null)
            {
                buffer = new ArraySegment<byte>(args.Buffer, args.Offset, args.Count);
                this.writeState.Args = args;
            }
            else
            {
                Fx.Assert(args.ByteBufferList != null, "Buffer list should not be null when buffer is null");
                if (args.ByteBufferList.Count == 1)
                {
                    ByteBuffer byteBuffer = args.ByteBufferList[0];
                    buffer = new ArraySegment<byte>(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
                    this.writeState.Args = args;
                }
                else
                {
                    // Copy all buffers into one big buffer to avoid SSL overhead
                    Fx.Assert(args.Count > 0, "args.Count should be set");
                    ByteBuffer temp = new ByteBuffer(args.Count, false, false);
                    for (int i = 0; i < args.ByteBufferList.Count; ++i)
                    {
                        ByteBuffer byteBuffer = args.ByteBufferList[i];
                        Buffer.BlockCopy(byteBuffer.Buffer, byteBuffer.Offset, temp.Buffer, temp.Length, byteBuffer.Length);
                        temp.Append(byteBuffer.Length);
                    }

                    buffer = new ArraySegment<byte>(temp.Buffer, 0, temp.Length);
                    this.writeState.Args = args;
                    this.writeState.Buffer = temp;
                }
            }

            var task = this.sslStream.WriteAsync(buffer.Array, buffer.Offset, buffer.Count);
            if (task.IsCompleted)
            {
                this.writeState.Reset();
                task.GetAwaiter().GetResult();
                args.CompletedSynchronously = true;
                args.BytesTransfered = buffer.Count;
                return false;
            }

            _ = task.ContinueWith((_t, _s) =>
            {
                var _this = (TlsTransport)_s;
                TransportAsyncCallbackArgs _args = _this.writeState.Args;
                _this.writeState.Reset();
                _args.CompletedSynchronously = false;
                if (_t.IsFaulted)
                {
                    _args.Exception = _t.Exception.InnerException;
                }
                else if (_t.IsCanceled)
                {
                    _args.Exception = new TaskCanceledException();
                }
                else
                {
                    _args.BytesTransfered = _args.Count;
                }
                _args.CompletedCallback.Invoke(_args);
            }, this);
            return true;
        }

        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            // Read with buffer list not supported
            Fx.Assert(args.Buffer != null, "must have buffer to read");
            Fx.Assert(this.readState.Args == null, "Cannot read when a read is still in progress");

            this.readState.Args = args;
            var task = this.sslStream.ReadAsync(args.Buffer, args.Offset, args.Count);
            if (task.IsCompleted)
            {
                this.readState.Reset();
                args.CompletedSynchronously = true;
                args.BytesTransfered = task.GetAwaiter().GetResult();
                return false;
            }

            _ = task.ContinueWith((_t, _s) =>
            {
                var _this = (TlsTransport)_s;
                var _args = _this.readState.Args;
                _this.readState.Reset();
                _args.CompletedSynchronously = false;
                if (_t.IsFaulted)
                {
                    _args.Exception = _t.Exception.InnerException;
                }
                else if (_t.IsCanceled)
                {
                    _args.Exception = new TaskCanceledException();
                }
                else
                {
                    _args.BytesTransfered = _t.Result;
                }
                _args.CompletedCallback.Invoke(_args);

            }, this);
            return true;
        }

        protected override bool OpenInternal()
        {
            IAsyncResult result;
            if (this.tlsSettings.IsInitiator)
            {
                X509CertificateCollection certCollection = new X509CertificateCollection();
                if (this.tlsSettings.Certificate != null)
                {
                    certCollection.Add(this.tlsSettings.Certificate);
                }

                result = this.BeginAuthenticateWithRetry(
                    this.tlsSettings.InternalProtocols, certCollection, this.tlsSettings.CheckCertificateRevocation,
                    (thisPtr, p, c, r) => thisPtr.sslStream.BeginAuthenticateAsClient(thisPtr.tlsSettings.TargetHost, c, p, r, onOpenComplete, thisPtr));
            }
            else
            {
                bool clientCertRequired = this.tlsSettings.CertificateValidationCallback != null;
                result = this.BeginAuthenticateWithRetry(
                    this.tlsSettings.InternalProtocols, clientCertRequired, this.tlsSettings.CheckCertificateRevocation,
                    (thisPtr, p, c, r) => thisPtr.sslStream.BeginAuthenticateAsServer(thisPtr.tlsSettings.Certificate, c, p, r, onOpenComplete, thisPtr));
            }

            bool completedSynchronously = result.CompletedSynchronously;
            if (completedSynchronously)
            {
                this.HandleOpenComplete(result, true);
            }

            return completedSynchronously;
        }

        protected override bool CloseInternal()
        {
            this.sslStream.Dispose();
            return true;
        }

        protected override void AbortInternal()
        {
            this.innerTransport.Abort();
        }

        protected virtual X509Principal CreateX509Principal(X509Certificate2 certificate)
        {
            return new X509Principal(new X509CertificateIdentity(certificate, this.sslStream.IsRemoteCertificateValid));
        }

        protected virtual bool ValidateRemoteCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            return this.tlsSettings.CertificateValidationCallback(sender, certificate, chain, sslPolicyErrors);
        }

        IAsyncResult BeginAuthenticateWithRetry<T1, T2>(SslProtocols sslProtocols, T1 t1, T2 t2, Func<TlsTransport, SslProtocols, T1, T2, IAsyncResult> func)
        {
            try
            {
                return func(this, sslProtocols, t1, t2);
            }
            catch (ArgumentException ae) when (string.Equals("sslProtocolType", ae.ParamName, StringComparison.Ordinal))
            {
                SslProtocols sslProtocols2 = this.tlsSettings.RefreshProtocolsOnArgumentError();
                if (sslProtocols2 == sslProtocols)
                {
                    throw;
                }

                return func(this, sslProtocols2, t1, t2);
            }
        }

        static void OnOpenComplete(IAsyncResult result)
        {
            if (!result.CompletedSynchronously)
            {
                TlsTransport thisPtr = (TlsTransport)result.AsyncState;
                thisPtr.HandleOpenComplete(result, false);
            }
        }

        void HandleOpenComplete(IAsyncResult result, bool syncComplete)
        {
            Exception exception = null;
            try
            {
                bool isInitiator = this.tlsSettings.IsInitiator;
                this.tlsSettings = null;
                if (isInitiator)
                {
                    this.sslStream.EndAuthenticateAsClient(result);
                }
                else
                {
                    this.sslStream.EndAuthenticateAsServer(result);
                    if (this.sslStream.RequireMutualAuthentication && this.sslStream.RemoteCertificate != null)
                    {
                        // Cannot cast from X509Certificate to X509Certificate2
                        // using workaround mentioned here: https://github.com/dotnet/corefx/issues/4510
                        var certificate = new X509Certificate2(sslStream.RemoteCertificate.Export(X509ContentType.Cert));
                        this.Principal = this.CreateX509Principal(certificate);
                    }
                }
            }
            catch (Exception exp) when (!Fx.IsFatal(exp))
            {
                if (syncComplete)
                {
                    throw;
                }

                exception = exp;
            }

            if (!syncComplete)
            {
                this.CompleteOpen(false, exception);
            }
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            this.sslStream?.Dispose();
        }

        struct OperationState
        {
            public TransportAsyncCallbackArgs Args;

            public ByteBuffer Buffer;

            public void Reset()
            {
                this.Buffer?.Dispose();
                this.Args = null;
                this.Buffer = null;
            }
        }
    }
}
