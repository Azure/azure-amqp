﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System.IO;
    using System.Net.Security;

    public sealed class CustomSslStream : SslStream
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
    }
}
