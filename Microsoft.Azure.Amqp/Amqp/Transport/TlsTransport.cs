// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net.Security;
    using System.Security.Cryptography.X509Certificates;
    using Microsoft.Azure.Amqp.X509;

    public class TlsTransport : TransportBase, IDisposable
    {
        static readonly AsyncCallback onOpenComplete = OnOpenComplete;
        static readonly AsyncCallback onWriteComplete = OnWriteComplete;
        static readonly AsyncCallback onReadComplete = OnReadComplete;
        readonly TransportBase innerTransport;
        readonly CustomSslStream sslStream;
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

            IAsyncResult result = this.sslStream.BeginWrite(buffer.Array, buffer.Offset, buffer.Count, onWriteComplete, this);
            bool completedSynchronously = result.CompletedSynchronously;
            if (completedSynchronously)
            {
                this.HandleOperationComplete(result, true, true);
            }

            return !completedSynchronously;
        }

        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            // Read with buffer list not supported
            Fx.Assert(args.Buffer != null, "must have buffer to read");
            Fx.Assert(this.readState.Args == null, "Cannot read when a read is still in progress");
            this.readState.Args = args;
            IAsyncResult result = this.sslStream.BeginRead(args.Buffer, args.Offset, args.Count, onReadComplete, this);
            bool completedSynchronously = result.CompletedSynchronously;
            if (completedSynchronously)
            {
                this.HandleOperationComplete(result, false, true);
            }

            return !completedSynchronously;
        }

        protected TlsTransportSettings TlsSettings
        {
            get { return this.tlsSettings; }
        }

        protected override bool OpenInternal()
        {
            IAsyncResult result;
            if (this.tlsSettings.IsInitiator)
            {
                bool checkRevocation = false;
                X509CertificateCollection certCollection = new X509CertificateCollection();
                if (this.tlsSettings.Certificate != null)
                {
                    certCollection.Add(this.tlsSettings.Certificate);
                    checkRevocation = true;
                }

                result = this.sslStream.BeginAuthenticateAsClient(this.tlsSettings.TargetHost,
                    certCollection, this.tlsSettings.Protocols, checkRevocation, onOpenComplete, this);
            }
            else
            {
                bool clientCert = this.tlsSettings.CertificateValidationCallback != null;
                result = this.sslStream.BeginAuthenticateAsServer(this.tlsSettings.Certificate,
                    clientCert, this.tlsSettings.Protocols, clientCert, onOpenComplete, this);
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

        static void OnOpenComplete(IAsyncResult result)
        {
            if (!result.CompletedSynchronously)
            {
                TlsTransport thisPtr = (TlsTransport)result.AsyncState;
                thisPtr.HandleOpenComplete(result, false);
            }
        }

        static void OnReadComplete(IAsyncResult result)
        {
            if (!result.CompletedSynchronously)
            {
                var thisPtr = (TlsTransport)result.AsyncState;
                thisPtr.HandleOperationComplete(result, false, false);
            }
        }

        static void OnWriteComplete(IAsyncResult result)
        {
            if (!result.CompletedSynchronously)
            {
                var thisPtr = (TlsTransport)result.AsyncState;
                thisPtr.HandleOperationComplete(result, true, false);
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
            catch (Exception exp)
            {
                if (Fx.IsFatal(exp) || syncComplete)
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

        void HandleOperationComplete(IAsyncResult result, bool write, bool syncComplete)
        {
            TransportAsyncCallbackArgs args = null;
            try
            {
                if (write)
                {
                    args = this.writeState.Args;
                    ByteBuffer buffer = this.writeState.Buffer;
                    this.writeState.Reset();

                    if (buffer != null)
                    {
                        buffer.Dispose();
                    }

                    this.sslStream.EndWrite(result);
                    args.BytesTransfered = args.Count;
                }
                else
                {
                    args = this.readState.Args;
                    this.readState.Reset();

                    args.BytesTransfered = this.sslStream.EndRead(result);
                }
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                args.Exception = exception;
            }

            args.CompletedSynchronously = syncComplete;

            if (!syncComplete)
            {
                Action<TransportAsyncCallbackArgs> callback = args.CompletedCallback;
                if (callback != null)
                {
                    args.CompletedCallback(args);
                }
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
                this.Args = null;
                this.Buffer = null;
            }
        }
    }
}
