// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    /// <summary>
    /// Defines the I/O events. Conditions for generating the
    /// events are specific to the source components.
    /// </summary>
    public enum IoEvent
    {
        /// <summary>
        /// The I/O queue is full.
        /// </summary>
        WriteBufferQueueFull,
        /// <summary>
        /// The I/O queue is empty.
        /// </summary>
        WriteBufferQueueEmpty
    }

    /// <summary>
    /// The handler for <see cref="IoEvent"/>.
    /// </summary>
    public interface IIoHandler
    {
        /// <summary>
        /// Creates a buffer for a given size.
        /// </summary>
        /// <param name="frameSize">The buffer size.</param>
        /// <returns>A <see cref="ByteBuffer"/> object.</returns>
        ByteBuffer CreateBuffer(int frameSize);

        /// <summary>
        /// Called when a buffer is received.
        /// </summary>
        /// <param name="buffer">The received buffer.</param>
        void OnReceiveBuffer(ByteBuffer buffer);

        /// <summary>
        /// Called when an I/O exception occurred.
        /// </summary>
        /// <param name="exception">The I/O exception.</param>
        void OnIoFault(Exception exception);

        /// <summary>
        /// Called when an I/O event is raised.
        /// </summary>
        /// <param name="ioEvent">The I/O event.</param>
        /// <param name="queueSize">The current queue size.</param>
        void OnIoEvent(IoEvent ioEvent, long queueSize);
    }
}
