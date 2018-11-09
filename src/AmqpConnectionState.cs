// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    public enum AmqpObjectState
    {
        // these states indicate that nothing has been received
        Start = 0,
        HeaderSent = 1,
        OpenPipe = 2,
        OpenClosePipe = 3,

        HeaderReceived = 4,
        HeaderExchanged = 5,
        OpenSent = 6,
        OpenReceived = 7,
        ClosePipe = 8,
        Opened = 9,
        CloseSent = 10,
        CloseReceived = 11,
        End = 12,
        Faulted = 13,
    }
}