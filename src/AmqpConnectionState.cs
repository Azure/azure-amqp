// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// State of an AMQP object.
    /// </summary>
    public enum AmqpObjectState
    {
        /// <summary>
        /// The object is just created.
        /// </summary>
        Start = 0,
        /// <summary>
        /// A protocol header was sent. Applicable to connection only.
        /// </summary>
        HeaderSent = 1,
        /// <summary>
        /// A protocol header and an open are sent. Applicable to connection only.
        /// </summary>
        OpenPipe = 2,
        /// <summary>
        /// A protocol header, an open and a close are sent. Applicable to connection only.
        /// </summary>
        OpenClosePipe = 3,

        /// <summary>
        /// A protocol header is received. Applicable to connection only.
        /// </summary>
        HeaderReceived = 4,
        /// <summary>
        /// Protocol header has been exchanged.
        /// </summary>
        HeaderExchanged = 5,
        /// <summary>
        /// An open/begin/attach is sent.
        /// </summary>
        OpenSent = 6,
        /// <summary>
        /// An open/begin/attach is received.
        /// </summary>
        OpenReceived = 7,
        /// <summary>
        /// A close/end/detach is sent right after open/begin/attach respectively.
        /// </summary>
        ClosePipe = 8,
        /// <summary>
        /// An open/begin/attach is exchanged with the peer.
        /// </summary>
        Opened = 9,
        /// <summary>
        /// A close/end/detach is sent.
        /// </summary>
        CloseSent = 10,
        /// <summary>
        /// A close/end/detach is received.
        /// </summary>
        CloseReceived = 11,
        /// <summary>
        /// A close/end/detach is exchanged with the peer.
        /// </summary>
        End = 12,
    }
}