// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.IO;
    using System.Net;
    using System.Net.Security;
    using System.Security.Authentication;
    using System.Security.Cryptography.X509Certificates;
    using Microsoft.Azure.Amqp.X509;

    /// <summary>
    /// Defines the TLS transport.
    /// </summary>
    public class TlsTransport : TransportBase, IDisposable
    {
        static readonly AsyncCallback onOpenComplete = OnOpenComplete;
        static readonly AsyncCallback onWriteComplete = OnWriteComplete;
        static readonly AsyncCallback onReadComplete = OnReadComplete;
        readonly TransportBase innerTransport;
        readonly TransportStream transportStream;
        /// <summary>The SSL stream (internal use only).</summary>
        protected readonly CustomSslStream sslStream;
        TlsTransportSettings tlsSettings;
        OperationState writeState;
        OperationState readState;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="innerTransport">The inner transport.</param>
        /// <param name="tlsSettings">The TLS transport settings.</param>
        public TlsTransport(TransportBase innerTransport, TlsTransportSettings tlsSettings)
            : base("tls", innerTransport.Identifier)
        {
            Fx.Assert((tlsSettings.IsInitiator && tlsSettings.TargetHost != null) || (!tlsSettings.IsInitiator && tlsSettings.Certificate != null),
                tlsSettings.IsInitiator ? "Must have a target host for the client." : "Must have a certificate for the server.");
            this.innerTransport = innerTransport;
            this.tlsSettings = tlsSettings;
            this.transportStream = new TransportStream(this.innerTransport);
            this.sslStream = tlsSettings.CertificateValidationCallback == null ?
                new CustomSslStream(this.transportStream, false, tlsSettings.IsInitiator) :
                new CustomSslStream(this.transportStream, false, this.ValidateRemoteCertificate, tlsSettings.IsInitiator);
        }

        /// <inheritdoc cref="TransportBase.Local"/>
        internal override EndPoint Local => this.innerTransport.Local;

        /// <inheritdoc cref="TransportBase.Remote"/>
        internal override EndPoint Remote => this.innerTransport.Remote;

        /// <inheritdoc/>
        public override string LocalEndPoint => this.innerTransport.LocalEndPoint;

        /// <inheritdoc/>
        public override string RemoteEndPoint => this.innerTransport.RemoteEndPoint;

        /// <summary>
        /// true since the transport is encrypted.
        /// </summary>
        public override bool IsSecure => true;

        /// <summary>
        /// Gets the SslStream of this transport.
        /// </summary>
        internal CustomSslStream SslStream
        {
            get { return this.sslStream; }
        }

        /// <summary>
        /// Sets a transport monitor for transport I/O operations.
        /// </summary>
        /// <param name="usageMeter">The transport monitor.</param>
        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.innerTransport.SetMonitor(usageMeter);
        }

        /// <summary>
        /// Starts a write operation.
        /// </summary>
        /// <param name="args">The write arguments.</param>
        /// <returns>true if the write operation is pending, otherwise false.</returns>
        public override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            Fx.Assert(this.writeState.Args == null, "Cannot write when a write is still in progress");
            this.writeState.Args = args;

            // Encrypt each source segment in place via a sync SslStream.Write and issue
            // a single async I/O to flush the accumulated ciphertext to the inner transport.
            IAsyncResult result;
            try
            {
                if (args.Buffer != null)
                {
                    this.sslStream.Write(args.Buffer, args.Offset, args.Count);
                }
                else
                {
                    Fx.Assert(args.ByteBufferList != null, "Buffer list should not be null when buffer is null");
                    for (int i = 0; i < args.ByteBufferList.Count; ++i)
                    {
                        ByteBuffer byteBuffer = args.ByteBufferList[i];
                        this.sslStream.Write(byteBuffer.Buffer, byteBuffer.Offset, byteBuffer.Length);
                    }
                }

                result = this.transportStream.BeginFlushWrite(onWriteComplete, this);
            }
            catch (ObjectDisposedException ode)
            {
                throw new IOException($"Transport '{this}' is closed", ode);
            }
            catch (InvalidOperationException ioe)
            {
                throw new IOException($"Transport '{this}' is valid for write operations.", ioe);
            }

            bool completedSynchronously = result.CompletedSynchronously;
            if (completedSynchronously)
            {
                this.HandleOperationComplete(result, true, true);
            }

            return !completedSynchronously;
        }

        /// <summary>
        /// Starts a read operation.
        /// </summary>
        /// <param name="args">The read arguments.</param>
        /// <returns>true if the read operation is pending, otherwise false.</returns>
        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            // Read with buffer list not supported
            Fx.Assert(args.Buffer != null, "must have buffer to read");
            Fx.Assert(this.readState.Args == null, "Cannot read when a read is still in progress");
            this.readState.Args = args;
            IAsyncResult result;
            try
            {
                result = this.sslStream.BeginRead(args.Buffer, args.Offset, args.Count, onReadComplete, this);
            }
            catch (ObjectDisposedException ode)
            {
                throw new IOException($"Transport '{this}' is closed", ode);
            }
            catch (InvalidOperationException ioe)
            {
                throw new IOException($"Transport '{this}' is valid for read operations.", ioe);
            }

            bool completedSynchronously = result.CompletedSynchronously;
            if (completedSynchronously)
            {
                this.HandleOperationComplete(result, false, true);
            }

            return !completedSynchronously;
        }

        /// <summary>
        /// Gets the TLS transport settings.
        /// </summary>
        protected TlsTransportSettings TlsSettings
        {
            get { return this.tlsSettings; }
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <returns>true if open is completed, otherwise false.</returns>
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

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>true if close is completed, otherwise false.</returns>
        protected override bool CloseInternal()
        {
            this.sslStream.Dispose();
            return true;
        }

        /// <summary>
        /// Aborts the object.
        /// </summary>
        protected override void AbortInternal()
        {
            this.innerTransport.Abort();
        }

        /// <summary>
        /// Creates a <see cref="X509Principal"/> from a certificate.
        /// </summary>
        /// <param name="certificate">The received certificate.</param>
        /// <returns>A <see cref="X509Principal"/> object.</returns>
        protected virtual X509Principal CreateX509Principal(X509Certificate2 certificate)
        {
            return new X509Principal(new X509CertificateIdentity(certificate, this.sslStream.IsRemoteCertificateValid));
        }

        /// <summary>
        /// Validates the remote certificate through <see cref="TlsTransportSettings.CertificateValidationCallback"/>.
        /// </summary>
        /// <param name="sender">The caller.</param>
        /// <param name="certificate">The certificate.</param>
        /// <param name="chain">The certificate chain.</param>
        /// <param name="sslPolicyErrors">The TLS policy errors.</param>
        /// <returns></returns>
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
            catch (Exception exp) when (!Fx.IsFatal(exp) && !syncComplete)
            {
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
                    this.writeState.Reset();

                    this.transportStream.EndWrite(result);
                    args.BytesTransfered = args.Count;
                }
                else
                {
                    args = this.readState.Args;
                    this.readState.Reset();

                    args.BytesTransfered = this.sslStream.EndRead(result);
                }
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                if (exception is InvalidOperationException)
                {
                    exception = new IOException($"Transport '{this}' is valid for IO operations.", exception);
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

            public void Reset()
            {
                this.Args = null;
            }
        }
    }
}
