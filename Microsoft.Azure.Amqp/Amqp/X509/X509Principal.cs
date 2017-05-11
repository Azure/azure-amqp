// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.X509
{
    using System;
    using System.Collections.Generic;
    using System.Security.Principal;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// Used to report a X509 certificate used as a security credential 
    /// </summary>
    public class X509Principal : IPrincipal
    {
        IList<X509ChainElement> certificateChainElements;

        /// <summary>
        /// Constructor which takes X509 certificate identity and a list of its certificate chain elements as input
        /// </summary>
        /// <param name="identity"></param>
        /// <param name="certificateChainElements"></param>
        public X509Principal(X509CertificateIdentity identity, IList<X509ChainElement> certificateChainElements)
        {
            this.CertificateIdentity = identity;
            this.certificateChainElements = certificateChainElements;
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
        ///  retrieve CertificateChainElements
        ///  use only once
        ///  will return null if invoked more than once 
        /// </summary>
        public IList<X509ChainElement> GetCertificateChainElementsAndRelease()
        {
            var certChainElements = this.certificateChainElements;
            // release local copy
            this.certificateChainElements = null;
            return certChainElements;
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
