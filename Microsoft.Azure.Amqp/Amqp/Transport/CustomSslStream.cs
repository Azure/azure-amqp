// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System.IO;
    using System.Net.Security;
    using System.Threading;
    using System.Threading.Tasks;

    sealed class CustomSslStream : SslStream
    {
        public CustomSslStream(Stream innerStream, bool leaveInnerStreamOpen, bool isClient)
            : base(innerStream, leaveInnerStreamOpen)
        {
            this.IsClient = isClient;
        }

        public CustomSslStream(Stream innerStream, bool leaveInnerStreamOpen, RemoteCertificateValidationCallback userCertificateValidationCallback, bool isClient)
            : base(innerStream, leaveInnerStreamOpen, userCertificateValidationCallback)
        {
            this.RequireMutualAuthentication = userCertificateValidationCallback != null;
            this.IsClient = isClient;
        }

        public bool IsClient
        {
            get;
            private set;
        }

        public bool RequireMutualAuthentication
        {
            get;
            private set;
        }

        public bool IsRemoteCertificateValid
        {
            get;
            internal set;
        }

#if DNXCORE
        // This is a work-around to a bug in CoreCLR where calling SslStream.ReadAsync (which doesn't complete yet)
        // followed by calling SslStream.WriteAsync the WriteAsync call blocks until the ReadAsync completes.
        // Work around this by overriding the base Stream.WriteAsync implementation to avoid this serialization.
        // Should be removed once we have the fix in SslStream and related classes.
        public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            base.Write(buffer, offset, count);
            return TaskHelpers.CompletedTask;
        }
#endif
    }
}
