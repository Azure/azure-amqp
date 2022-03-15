// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// ICbsTokenProvider interface
    /// </summary>
    public interface ICbsTokenProvider
    {
        /// <summary>
        /// Generate a Token for use with CBS.
        /// </summary>
        Task<CbsToken> GetTokenAsync(Uri namespaceAddress, string appliesTo, string[] requiredClaims);
    }
}
