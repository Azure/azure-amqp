// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.X509
{
    using System;
    using System.Security.Principal;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// Used to report a X509 certificate used as a security credential 
    /// </summary>
    public class X509Principal : IPrincipal
    {
        X509Chain certificateChain;

        /// <summary>
        /// Constructor which takes X509 certificate identity and its certificate chain as input
        /// </summary>
        /// <param name="identity"></param>
        /// <param name="certificateChain"></param>
        public X509Principal(X509CertificateIdentity identity, X509Chain certificateChain)
        {
            this.CertificateIdentity = identity;
            this.certificateChain = certificateChain;
        }

        /// <summary>
        /// Get Accessor
        /// </summary>
        public IIdentity Identity => this.CertificateIdentity;

        /// <summary>
        ///  CertificateIdentity get accessor
        /// </summary>
        public X509CertificateIdentity CertificateIdentity { get; }

        /// <summary>
        ///  retrieve CertificateChain 
        ///  use only once
        ///  will return null if invoked more than once 
        /// </summary>
        public X509Chain GetCertificateChain()
        {
            var certChain = this.certificateChain;
            // release local copy
            this.certificateChain = null;
            return certChain;
        }

        /// <summary>
        ///  Method not implemented
        /// </summary>
        /// <param name="role"></param>
        /// <returns></returns>
        public bool IsInRole(string role)
        {
            throw new InvalidOperationException();
        }
    }
}
