// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    /// <summary>
    /// Encapsulates a token for use with AMQP CBS
    /// </summary>
    public class CbsToken
    {
        /// <summary>
        /// Constructs a new instance with the provided values.
        /// </summary>
        public CbsToken(object tokenValue, string tokenType, DateTime expiresAtUtc)
        {
            if (tokenValue == null)
            {
                throw new ArgumentNullException("tokenValue");
            }
            else if (string.IsNullOrWhiteSpace(tokenType))
            {
                throw new ArgumentNullException("tokenType");
            }

            this.TokenValue = tokenValue;
            this.TokenType = tokenType;
            this.ExpiresAtUtc = expiresAtUtc;
        }

        /// <summary>
        /// The token itself
        /// </summary>
        public object TokenValue { get; private set; }

        /// <summary>
        /// The type of this token
        /// </summary>
        public string TokenType { get; private set; }

        /// <summary>
        /// The expiration time of the token
        /// </summary>
        public DateTime ExpiresAtUtc { get; private set; }
    }
}
