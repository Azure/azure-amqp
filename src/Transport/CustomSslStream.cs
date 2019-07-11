// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System.IO;
    using System.Net.Security;

    /// <summary>
    /// An <see cref="SslStream"/> with extended properties. Used by <see cref="TlsTransport"/>.
    /// </summary>
    public sealed class CustomSslStream : SslStream
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="innerStream">The inner transport stream.</param>
        /// <param name="leaveInnerStreamOpen">true to keep inner stream open when this stream is disposed.</param>
        /// <param name="isClient">true if this is a TLS client, false otherwise.</param>
        public CustomSslStream(Stream innerStream, bool leaveInnerStreamOpen, bool isClient)
            : base(innerStream, leaveInnerStreamOpen)
        {
            this.IsClient = isClient;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="innerStream">The inner transport stream.</param>
        /// <param name="leaveInnerStreamOpen">true to keep inner stream open when this stream is disposed.</param>
        /// <param name="userCertificateValidationCallback">The user's <see cref="RemoteCertificateValidationCallback"/>.</param>
        /// <param name="isClient">true if this is a TLS client, false otherwise.</param>
        public CustomSslStream(Stream innerStream, bool leaveInnerStreamOpen, RemoteCertificateValidationCallback userCertificateValidationCallback, bool isClient)
            : base(innerStream, leaveInnerStreamOpen, userCertificateValidationCallback)
        {
            this.RequireMutualAuthentication = userCertificateValidationCallback != null;
            this.IsClient = isClient;
        }

        /// <summary>
        /// Gets a boolean value that indicates if this is the TLS client.
        /// </summary>
        public bool IsClient
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a boolean value that indicates if the stream requires mutual authentication.
        /// </summary>
        public bool RequireMutualAuthentication
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a boolean value that indicates if the remote certificate is valid.
        /// </summary>
        public bool IsRemoteCertificateValid
        {
            get;
            internal set;
        }
    }
}
