// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.X509
{
    using System;
    using System.Security.Principal;

    /// <summary>
    /// Used to report a X509 certificate used as a security credential 
    /// </summary>
    public class X509Principal : IPrincipal
    {
        /// <summary>
        /// Ctor which takes identity as input
        /// </summary>
        /// <param name="identity"></param>
        public X509Principal(X509CertificateIdentity identity)
        {
            this.CertificateIdentity = identity;
        }

        /// <summary>
        /// Get Accessor
        /// </summary>
        public IIdentity Identity => this.CertificateIdentity;

        /// <summary>
        ///  Type specific get accessor
        /// </summary>
        public X509CertificateIdentity CertificateIdentity { get; }

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
