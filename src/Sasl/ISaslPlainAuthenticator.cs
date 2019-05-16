// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System.Security.Principal;
    using System.Threading.Tasks;

    /// <summary>
    /// Defines the interface of an authenticator that authenticates
    /// SASL PLAIN credentials.
    /// </summary>
    public interface ISaslPlainAuthenticator
    {
        /// <summary>
        /// Authenticates the credentials from SASL PLAIN.
        /// </summary>
        /// <param name="identity">The identity name.</param>
        /// <param name="password">The password.</param>
        /// <returns>A task that completes with an <see cref="IPrincipal"/> as
        /// the authentication result.</returns>
        Task<IPrincipal> AuthenticateAsync(string identity, string password);
    }
}
