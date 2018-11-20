// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Constants useful for CBS scenarios
    /// </summary>
    public static class CbsConstants
    {
        /// <summary>
        /// The Property name for setting timeouts
        /// </summary>
        public static readonly AmqpSymbol TimeoutName = AmqpConstants.Vendor + ":timeout";

        /// <summary>
        /// The address of the CBS Node ($cbs)
        /// </summary>
        public const string CbsAddress = "$cbs";

        /// <summary>
        /// The operation property name
        /// </summary>
        public const string Operation = "operation";

        public static class PutToken
        {
            /// <summary>
            /// The put-token operation property value
            /// </summary>
            public const string OperationValue = "put-token";

            /// <summary>
            /// The token type property name
            /// </summary>
            public const string Type = "type";

            /// <summary>
            /// The audience property name
            /// </summary>
            public const string Audience = "name";

            /// <summary>
            /// The expiration property name
            /// </summary>
            internal const string Expiration = "expiration";

            /// <summary>
            /// The response status code property name
            /// </summary>
            public const string StatusCode = "status-code";

            /// <summary>
            /// The response status description property name
            /// </summary>
            public const string StatusDescription = "status-description";
        }
    }
}
