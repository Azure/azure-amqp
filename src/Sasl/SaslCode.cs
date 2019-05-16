// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    /// <summary>
    /// Defines the codes that indicate the outcome of the sasl dialog.
    /// </summary>
    public enum SaslCode : byte
    {
        /// <summary>
        /// Connection authentication succeeded.
        /// </summary>
        Ok = 0,
        /// <summary>
        /// Connection authentication failed due to an unspecified problem with the supplied credentials.
        /// </summary>
        Auth = 1,
        /// <summary>
        /// Connection authentication failed due to a system error.
        /// </summary>
        Sys = 2,
        /// <summary>
        /// Connection authentication failed due to a system error that is unlikely to be corrected without intervention.
        /// </summary>
        SysPerm = 3,
        /// <summary>
        /// Connection authentication failed due to a transient system error.
        /// </summary>
        SysTemp = 4
    }
}
