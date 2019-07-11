// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    /// <summary>
    /// Defines the frame type.
    /// </summary>
    public enum FrameType : byte
    {
        /// <summary>AMQP frame.</summary>
        Amqp = 0,
        /// <summary>SASL frame.</summary>
        Sasl = 1
    }
}
