// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.X509
{
    using System;
    using System.Security.Principal;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// Represents an X509 certificate as a security identity
    /// </summary>
    public class X509CertificateIdentity : IIdentity
    {
        /// <summary>
        /// Maximum Clock Skew allowed
        /// </summary>
        public static readonly TimeSpan MaxClockSkew = TimeSpan.FromMinutes(5);

        /// <summary>
        /// ctor which takes a X509 certificate
        /// </summary>
        /// <param name="certificate"></param>
        public X509CertificateIdentity(X509Certificate2 certificate)
        {
            this.Certificate = certificate;
            this.Name = this.Certificate.Subject;
            this.AuthenticationType = "X509Certificate";
        }

        /// <summary>
        /// Name accessor
        /// </summary>
        public string Name { get; }

        /// <summary>
        ///  Authentication type accessor
        /// </summary>
        public string AuthenticationType { get; }

        /// <summary>
        ///  IsAuthenticated accessor
        /// </summary>
        public bool IsAuthenticated => false;

        /// <summary>
        ///  Actual X509 Certificate
        /// </summary>
        public X509Certificate2 Certificate { get; }

        /// <summary>
        ///  Check is the certificate has expired or is too new
        /// </summary>
        /// <returns></returns>
        public bool IsExpiredOrNotValidYet()
        {
            var currentTime = DateTime.Now;
            return (this.Certificate.NotAfter + MaxClockSkew < currentTime ||
                    this.Certificate.NotBefore - MaxClockSkew > currentTime);
        }
    }
}