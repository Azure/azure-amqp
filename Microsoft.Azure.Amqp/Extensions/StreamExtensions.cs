// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.IO;
    using System.Net.Security;
    using System.Security.Authentication;
    using System.Security.Cryptography.X509Certificates;

    static class StreamExtensions
    {
// DNXCORE50 removes Stream's BeginRead/EndRead/BeginWrite/EndWrite as well as SslStream IAsyncResult methods.
// Define them here in terms of the Task *Async versions to allow cross-compilation.
#if DNXCORE
        public static IAsyncResult BeginRead(this Stream stream, byte[] buffer, int offset, int count, AsyncCallback callback, object state)
        {
            return stream.ReadAsync(buffer, offset, count).ToAsyncResult<int>(callback, state);
        }

        public static int EndRead(this Stream stream, IAsyncResult asyncResult)
        {
            return TaskHelpers.EndAsyncResult<int>(asyncResult);
        }

        public static IAsyncResult BeginWrite(this Stream stream, byte[] buffer, int offset, int count, AsyncCallback callback, object state)
        {
            return stream.WriteAsync(buffer, offset, count).ToAsyncResult(callback, state);
        }

        public static void EndWrite(this Stream stream, IAsyncResult asyncResult)
        {
            TaskHelpers.EndAsyncResult(asyncResult);
        }

        public static IAsyncResult BeginAuthenticateAsClient(this SslStream sslStream, string targetHost, AsyncCallback callback, object state)
        {
            return sslStream.AuthenticateAsClientAsync(targetHost).ToAsyncResult(callback, state);
        }

        public static IAsyncResult BeginAuthenticateAsClient(
            this SslStream sslStream,
            string targetHost,
            X509CertificateCollection clientCerts,
            SslProtocols enabledProtocols,
            bool checkRevocation,
            AsyncCallback callback,
            object state)
        {
            return sslStream.AuthenticateAsClientAsync(targetHost, clientCerts, enabledProtocols, checkRevocation).ToAsyncResult(callback, state);
        }

        public static void EndAuthenticateAsClient(this SslStream sslStream, IAsyncResult asyncResult)
        {
            TaskHelpers.EndAsyncResult(asyncResult);
        }

        public static IAsyncResult BeginAuthenticateAsServer(
            this SslStream sslStream, X509Certificate serverCertificate, AsyncCallback callback, object state)
        {
            return sslStream.AuthenticateAsServerAsync(serverCertificate).ToAsyncResult(callback, state);
        }

        public static IAsyncResult BeginAuthenticateAsServer(
            this SslStream sslStream,
            X509Certificate serverCert,
            bool clientCertRequired,
            SslProtocols enabledProtocols,
            bool checkRevocation,
            AsyncCallback callback,
            object state)
        {
            return sslStream.AuthenticateAsServerAsync(
                serverCert, clientCertRequired, enabledProtocols, checkRevocation).ToAsyncResult(callback, state);
        }

        public static void EndAuthenticateAsServer(this SslStream sslStream, IAsyncResult asyncResult)
        {
            TaskHelpers.EndAsyncResult(asyncResult);
        }

#endif // DNXCORE
    }
}
