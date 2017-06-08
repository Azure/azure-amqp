// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using Windows.Networking;
    using Windows.Networking.Sockets;

    public class TlsTransport : TransportBase
    {
        readonly TcpTransport innerTransport;
        readonly TlsTransportSettings tlsSettings;

        public TlsTransport(TransportBase innerTransport, TlsTransportSettings tlsSettings)
            : base("tls", innerTransport.Identifier)
        {
            this.innerTransport = (TcpTransport)innerTransport;
            this.tlsSettings = tlsSettings;
        }

        public override string LocalEndPoint
        {
            get { return this.innerTransport.LocalEndPoint; }
        }

        public override string RemoteEndPoint
        {
            get { return this.innerTransport.RemoteEndPoint; }
        }

        public override bool IsSecure
        {
            get { return true; }
        }

        public override void SetMonitor(ITransportMonitor usageMeter)
        {
            this.innerTransport.SetMonitor(usageMeter);
        }

        public override bool WriteAsync(TransportAsyncCallbackArgs args)
        {
            return this.innerTransport.WriteAsync(args);
        }

        public override bool ReadAsync(TransportAsyncCallbackArgs args)
        {
            return this.innerTransport.ReadAsync(args);
        }

        protected override bool OpenInternal()
        {
            var task = this.innerTransport.Socket.UpgradeToSslAsync(SocketProtectionLevel.Tls12, new HostName(this.tlsSettings.TargetHost)).AsTask();
            if (task.IsCompleted)
            {
                return false;
            }

            task.ContinueWith(_t =>
            {
                if (_t.IsFaulted)
                {
                    this.CompleteOpen(false, _t.Exception);
                }
                else if (_t.IsCanceled)
                {
                    this.CompleteOpen(false, new OperationCanceledException());
                }
                else
                {
                    this.CompleteOpen(false, null);
                }
            });
            return true;
        }

        protected override bool CloseInternal()
        {
            this.innerTransport.SafeClose();
            return true;
        }

        protected override void AbortInternal()
        {
            this.innerTransport.Abort();
        }
    }
}
