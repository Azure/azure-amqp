// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// Defines whether and how frames should be traced. Handles AMQP protocol events.
    /// </summary>
    public class AmqpTrace
    {
        /// <summary>
        /// An instance of AmqpTrace to handle protocol events.
        /// </summary>
        public static AmqpTrace Provider = new AmqpTrace();

        /// <summary>
        /// A boolean value that controls if frames should be traced. It is set when
        /// the value of environment variable "AMQP_DEBUG" is "1".
        /// </summary>
        /// <remarks>When the flag is true, the library performs the following actioins
        /// for every frame sent and received.
        /// 1. Invoke FrameCallback if it is set, or
        /// 2. Create a string representation of the frame,
        /// 3. Invoke TraceCallback if it is set, or
        /// 4. Write the string to <see cref="System.Diagnostics.Debug"/> channel.
        /// These actions may have significant performance impact, so the flag should
        /// be turned on only for debugging purposes.
        /// </remarks>
        public static bool AmqpDebug = string.Equals(Environment.GetEnvironmentVariable("AMQP_DEBUG"), "1", StringComparison.Ordinal);
        /// <summary>
        /// The callback that is invoked when a frame is sent or received.
        /// </summary>
        public static Action<bool, AmqpConnection, Frame> FrameCallback;
        /// <summary>
        /// The callback that is invoked with a string representation of a frame.
        /// </summary>
        public static Action<string> TraceCallback;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        protected AmqpTrace()
        {
        }

        /// <summary>
        /// Called when a connection is opening.
        /// </summary>
        /// <param name="connection">The connection.</param>
        public virtual void AmqpOpenConnection(AmqpConnection connection)
        {
        }

        /// <summary>
        /// Called when a connection is closing or aborting.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="abort">true if the connection is aborted.</param>
        public virtual void AmqpCloseConnection(AmqpConnection connection, bool abort)
        {
        }

        /// <summary>
        /// Called when a session is being added to a connection.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="session">The session.</param>
        /// <param name="localChannel">Local channel number of the session.</param>
        /// <param name="remoteChannel">Remote channel number of the session.</param>
        public virtual void AmqpAddSession(AmqpConnection connection, AmqpSession session, ushort localChannel, ushort remoteChannel)
        {
        }

        /// <summary>
        /// Called when a link is being added to a session.
        /// </summary>
        /// <param name="session">The session.</param>
        /// <param name="link">The lin.</param>
        /// <param name="localHandle">Local handle of the link.</param>
        /// <param name="remoteHandle">Remote handle of the link.</param>
        /// <param name="linkName">Name of the link.</param>
        /// <param name="role">Role of the link.</param>
        /// <param name="source"><see cref="Source"/> of the link.</param>
        /// <param name="target"><see cref="Target"/> of the link.</param>
        public virtual void AmqpAttachLink(AmqpSession session, AmqpLink link, uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
        }

        /// <summary>
        /// Called when a delivery is not found in the unsettled map of a link.
        /// </summary>
        /// <param name="link">The link.</param>
        /// <param name="deliveryTag">See <see cref="Delivery.DeliveryTag"/> for details.</param>
        public virtual void AmqpDeliveryNotFound(AmqpLink link, string deliveryTag)
        {
        }

        /// <summary>
        /// Called when the state of a delivery is being updated.
        /// </summary>
        /// <param name="link">The link over which the delivery is sent or received.</param>
        /// <param name="deliveryId">See <see cref="Delivery.DeliveryId"/>.</param>
        /// <param name="settled">true if the delivery is settled, or false otherwise.</param>
        /// <param name="state">The delivery state.</param>
        public virtual void AmqpDispose(AmqpLink link, uint deliveryId, bool settled, object state)
        {
        }

        /// <summary>
        /// Called when a transport buffer size changes.
        /// </summary>
        /// <param name="transport">The transport that uses the buffer.</param>
        /// <param name="type">The type of the transport.</param>
        /// <param name="oldSize">The previous buffer size.</param>
        /// <param name="newSize">The current buffer size.</param>
        public virtual void AmqpDynamicBufferSizeChange(TransportBase transport, string type, int oldSize, int newSize)
        {
        }

        /// <summary>
        /// Called when an insecure transport is accepted.
        /// </summary>
        /// <param name="listener">The listener that accepted the transport.</param>
        /// <param name="transport">The accepted transport.</param>
        /// <param name="isSecure">true if the transport is encrypted, or false otherwise.</param>
        /// <param name="isAuthenticated">true if the transport is authenticated, or false otherwise.</param>
        public virtual void AmqpInsecureTransport(AmqpTransportListener listener, TransportBase transport, bool isSecure, bool isAuthenticated)
        {
        }

        /// <summary>
        /// Called when a link is being detached.
        /// </summary>
        /// <param name="link">The link.</param>
        /// <param name="name">Name of the link.</param>
        /// <param name="handle">Local handle of the link.</param>
        /// <param name="action">The action causing the deatch.</param>
        /// <param name="error">Error message associated with the detach.</param>
        public virtual void AmqpLinkDetach(AmqpLink link, string name, uint handle, string action, string error)
        {
        }

        /// <summary>
        /// Called when an error occurs when accepting a socket.
        /// </summary>
        /// <param name="listener">The transport listener.</param>
        /// <param name="willRetry">true if the listener will retry, or false otherwise.</param>
        /// <param name="error">The error message.</param>
        public virtual void AmqpListenSocketAcceptError(TransportListener listener, bool willRetry, string error)
        {
        }

        /// <summary>
        /// Called when an error occurrs while executing an operation.
        /// </summary>
        /// <param name="source">The object where the error comes from.</param>
        /// <param name="operation">The operation.</param>
        /// <param name="exception">The exception.</param>
        public virtual void AmqpLogError(object source, string operation, Exception exception)
        {
        }

        /// <summary>
        /// Called when an operation is being executed for informational logging.
        /// </summary>
        /// <param name="source">The object where the operation is performed.</param>
        /// <param name="operation">The operation.</param>
        /// <param name="detail">The object that is associated with the operation.</param>
        public virtual void AmqpLogOperationInformational(object source, TraceOperation operation, object detail)
        {
        }

        /// <summary>
        /// Called when an operation is being executed for verbose logging.
        /// </summary>
        /// <param name="source">The object where the operation is performed.</param>
        /// <param name="operation">The operation.</param>
        /// <param name="detail">The object that is associated with the operation.</param>
        public virtual void AmqpLogOperationVerbose(object source, TraceOperation operation, object detail)
        {
        }

        /// <summary>
        /// Called when a channel number is not found in a connection or a link handle is not found in a session.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="session">Null if it is for a session channel, or the session of the missing link handle.</param>
        /// <param name="type">Type of the handle ("session" or "link").</param>
        /// <param name="handle">The session channel or the link handle.</param>
        public virtual void AmqpMissingHandle(AmqpConnection connection, AmqpSession session, string type, uint handle)
        {
        }

        /// <summary>
        /// Called when a link attach fails.
        /// </summary>
        /// <param name="source">The object that generated the event.</param>
        /// <param name="name">Name of the source object.</param>
        /// <param name="entityName">The entity name (typically the address where the link attaches).</param>
        /// <param name="error">The error.</param>
        public virtual void AmqpOpenEntityFailed(object source, string name, string entityName, Exception error)
        {
        }

        /// <summary>
        /// Called when a link attach succeeds.
        /// </summary>
        /// <param name="source">The object that generated the event.</param>
        /// <param name="name">Name of the source object.</param>
        /// <param name="entityName">The entity name (typically the address where the link attaches).</param>
        public virtual void AmqpOpenEntitySucceeded(object source, string name, string entityName)
        {
        }

        /// <summary>
        /// Called when a message is received.
        /// </summary>
        /// <param name="link">The receiving link.</param>
        /// <param name="deliveryId">The delivery id of the message.</param>
        /// <param name="transferCount">Number of transfers of the message.</param>
        public virtual void AmqpReceiveMessage(AmqpLink link, uint deliveryId, int transferCount)
        {
        }

        /// <summary>
        /// Called when a link is being removed from a session.
        /// </summary>
        /// <param name="session">The session.</param>
        /// <param name="link">The link.</param>
        /// <param name="localHandle">Local handle of the link.</param>
        /// <param name="remoteHandle">Remote handle of the link.</param>
        /// <param name="linkName">Name of the link.</param>
        public virtual void AmqpRemoveLink(AmqpSession session, object link, uint localHandle, uint remoteHandle, string linkName)
        {
        }

        /// <summary>
        /// Called when a session is being removed from a connection.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="session">The session.</param>
        /// <param name="localChannel">Local channel of the session.</param>
        /// <param name="remoteChannel">Remote channel of the session.</param>
        public virtual void AmqpRemoveSession(AmqpConnection connection, AmqpSession session, ushort localChannel, ushort remoteChannel)
        {
        }

        /// <summary>
        /// Called when a session window is 0 when a tranfer is sent or received.
        /// </summary>
        /// <param name="session">The session.</param>
        /// <param name="nextId">Next transfer id.</param>
        public virtual void AmqpSessionWindowClosed(AmqpSession session, int nextId)
        {
        }

        /// <summary>
        /// Called when an AmqpObject state is changed.
        /// </summary>
        /// <param name="source">The object.</param>
        /// <param name="operation">The operation that triggers the change.</param>
        /// <param name="fromState">The previous state.</param>
        /// <param name="toState">The current state.</param>
        public virtual void AmqpStateTransition(AmqpObject source, string operation, AmqpObjectState fromState, AmqpObjectState toState)
        {
        }

        /// <summary>
        /// Called when a transport is being upgraded.
        /// </summary>
        /// <param name="source">The object that sends the event.</param>
        /// <param name="from">The previous transport.</param>
        /// <param name="to">The current transport.</param>
        public virtual void AmqpUpgradeTransport(object source, TransportBase from, TransportBase to)
        {
        }

        /// <summary>
        /// Called when an exception is thrown from the <see cref="AmqpObject.Abort"/> method.
        /// </summary>
        /// <param name="exception">The exception.</param>
        public virtual void AmqpAbortThrowingException(Exception exception)
        {
        }

        /// <summary>
        /// Called when a message is received and put into the local buffer queue because
        /// there is no receive request and message callback.
        /// </summary>
        /// <param name="link">The receiving link.</param>
        /// <param name="deliveryId">Delivery id of the message.</param>
        /// <param name="count">Number of messages in the buffer queue.</param>
        /// <param name="isPrefecthingBySize">true if the buffer queue is limited by size.</param>
        /// <param name="totalCacheSizeInBytes">The buffer queue limit.</param>
        /// <param name="totalLinkCredit">The total credit set on the link.</param>
        /// <param name="linkCredit">The current link credit of the link.</param>
        public virtual void AmqpCacheMessage(AmqpLink link, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
        }

        /// <summary>
        /// Called when an IoEvent occurrs.
        /// </summary>
        /// <param name="source">The object that sends the event.</param>
        /// <param name="ioEvent">The event.</param>
        /// <param name="queueSize">The size that the source maintains for such events.</param>
        public virtual void AmqpIoEvent(AmqpObject source, IoEvent ioEvent, long queueSize)
        {
        }
    }
}
