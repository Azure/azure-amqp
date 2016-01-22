// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System.Security.Principal;
    using System.Threading.Tasks;

    public interface ISaslPlainAuthenticator
    {
        Task<IPrincipal> AuthenticateAsync(string identity, string password);
    }
}
