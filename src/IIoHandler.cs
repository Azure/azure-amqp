// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    public enum IoEvent
    {
        WriteBufferQueueFull,
        WriteBufferQueueEmpty
    }

    public interface IIoHandler
    {
        ByteBuffer CreateBuffer(int frameSize);

        void OnReceiveBuffer(ByteBuffer buffer);

        void OnIoFault(Exception exception);

        void OnIoEvent(IoEvent ioEvent, long queueSize);
    }
}
