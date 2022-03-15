// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics.Tracing;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// Handles AMQP protocol events and writes <see cref="EventSource"/> events by default.
    /// </summary>
    public class AmqpTrace
    {
        /// <summary>
        /// An instance of AmqpTrace to handle protocol events.
        /// </summary>
        public static AmqpTrace Provider = new AmqpTrace();

        /// <summary>
        /// The callback that is invoked with a string representation of a frame.
        /// </summary>
        public static Action<string> FrameLogger;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        protected AmqpTrace()
        {
        }

        /// <summary>
        /// Called when a connection is opening.
        /// </summary>
        /// <param name="source">The object that calls this method.</param>
        /// <param name="connection">The connection.</param>
        public virtual void AmqpOpenConnection(object source, AmqpConnection connection)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenConnection(source.ToString(), connection.ToString());
            }
        }

        /// <summary>
        /// Called when a connection is closing or aborting.
        /// </summary>
        /// <param name="source">The object that calls this method.</param>
        /// <param name="connection">The connection.</param>
        /// <param name="abort">true if the connection is aborted.</param>
        public virtual void AmqpCloseConnection(object source, AmqpConnection connection, bool abort)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCloseConnection(source.ToString(), connection.ToString(), abort);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAddSession(connection.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        /// <summary>
        /// Called when a link is being added to a session.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="session">The session.</param>
        /// <param name="link">The lin.</param>
        /// <param name="localHandle">Local handle of the link.</param>
        /// <param name="remoteHandle">Remote handle of the link.</param>
        /// <param name="linkName">Name of the link.</param>
        /// <param name="role">Role of the link.</param>
        /// <param name="source"><see cref="Source"/> of the link.</param>
        /// <param name="target"><see cref="Target"/> of the link.</param>
        public virtual void AmqpAttachLink(AmqpConnection connection, AmqpSession session, AmqpLink link,
            uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAttachLink(connection.ToString(), session.ToString(), link.ToString(),
                    localHandle, remoteHandle, linkName, role, string.Empty);
            }
        }

        /// <summary>
        /// Called when a delivery is not found in the unsettled map of a link.
        /// </summary>
        /// <param name="link">The link.</param>
        /// <param name="deliveryTag">See <see cref="Delivery.DeliveryTag"/> for details.</param>
        public virtual void AmqpDeliveryNotFound(AmqpLink link, string deliveryTag)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDeliveryNotFound(link.ToString(), deliveryTag);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDispose(link.ToString(), deliveryId, settled, state?.ToString() ?? string.Empty);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDynamicBufferSizeChange(transport.ToString(), type, oldSize, newSize);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpInsecureTransport(listener.ToString(), transport.ToString(), isSecure, isAuthenticated);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLinkDetach(link.ToString(), name, handle, action, error);
            }
        }

        /// <summary>
        /// Called when an error occurs when accepting a socket.
        /// </summary>
        /// <param name="listener">The transport listener.</param>
        /// <param name="willRetry">true if the listener will retry, or false otherwise.</param>
        /// <param name="error">The error message.</param>
        public virtual void AmqpListenSocketAcceptError(TransportListener listener, bool willRetry, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpListenSocketAcceptError(listener.ToString(), willRetry, error);
            }
        }

        /// <summary>
        /// Called when an error occurrs while executing an operation.
        /// </summary>
        /// <param name="source">The object where the error comes from.</param>
        /// <param name="operation">The operation.</param>
        /// <param name="exception">The exception.</param>
        public virtual void AmqpLogError(object source, string operation, Exception exception)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogError(source.ToString(), operation, exception.ToString());
            }
        }

        /// <summary>
        /// Called when an operation is being executed for informational logging.
        /// </summary>
        /// <param name="source">The object where the operation is performed.</param>
        /// <param name="operation">The operation.</param>
        /// <param name="detail">The object that is associated with the operation.</param>
        public virtual void AmqpLogOperationInformational(object source, TraceOperation operation, object detail)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogOperationInformational(source.ToString(), operation, detail.ToString());
            }
        }

        /// <summary>
        /// Called when an operation is being executed for verbose logging.
        /// </summary>
        /// <param name="source">The object where the operation is performed.</param>
        /// <param name="operation">The operation.</param>
        /// <param name="detail">The object that is associated with the operation.</param>
        public virtual void AmqpLogOperationVerbose(object source, TraceOperation operation, object detail)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogOperationVerbose(source.ToString(), operation, detail.ToString());
            }
        }

        /// <summary>
        /// Called when a channel number is not found in a connection or a link handle is not found in a session.
        /// </summary>
        /// <param name="container">The <see cref="AmqpObject"/> that owns the handle table.</param>
        /// <param name="type">Type of the handle ("session" or "link").</param>
        /// <param name="handle">The session channel or the link handle.</param>
        public virtual void AmqpMissingHandle(AmqpObject container, string type, uint handle)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Warning, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpMissingHandle(container.ToString(), type, handle);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntityFailed(source.ToString(), name, entityName, error.ToString());
            }
        }

        /// <summary>
        /// Called when a link attach succeeds.
        /// </summary>
        /// <param name="source">The object that generated the event.</param>
        /// <param name="name">Name of the source object.</param>
        /// <param name="entityName">The entity name (typically the address where the link attaches).</param>
        public virtual void AmqpOpenEntitySucceeded(object source, string name, string entityName)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntitySucceeded(source.ToString(), name, entityName);
            }
        }

        /// <summary>
        /// Called when a message is received.
        /// </summary>
        /// <param name="link">The sending link.</param>
        /// <param name="deliveryId">The delivery id of the message.</param>
        /// <param name="bytes">Total bytes transferred.</param>
        public virtual void AmqpSentMessage(AmqpLink link, uint deliveryId, long bytes)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSentMessage(link.ToString(), deliveryId, bytes);
            }
        }

        /// <summary>
        /// Called when a message is received.
        /// </summary>
        /// <param name="link">The receiving link.</param>
        /// <param name="deliveryId">The delivery id of the message.</param>
        /// <param name="transferCount">Number of transfers of the message.</param>
        public virtual void AmqpReceiveMessage(AmqpLink link, uint deliveryId, int transferCount)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpReceiveMessage(link.ToString(), deliveryId, transferCount);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveLink(session.Connection.ToString(), session.ToString(), link.ToString(), localHandle, remoteHandle, linkName);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveSession(connection.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        /// <summary>
        /// Called when a session window is 0 when a tranfer is sent or received.
        /// </summary>
        /// <param name="session">The session.</param>
        /// <param name="nextId">Next transfer id.</param>
        public virtual void AmqpSessionWindowClosed(AmqpSession session, int nextId)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSessionWindowClosed(session.ToString(), nextId);
            }
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
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpStateTransition(source.ToString(), operation, fromState.ToString(), toState.ToString());
            }
        }

        /// <summary>
        /// Called when a transport is being upgraded.
        /// </summary>
        /// <param name="source">The object that sends the event.</param>
        /// <param name="from">The previous transport.</param>
        /// <param name="to">The current transport.</param>
        public virtual void AmqpUpgradeTransport(object source, TransportBase from, TransportBase to)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpUpgradeTransport(source.ToString(), from.ToString(), to.ToString());
            }
        }

        /// <summary>
        /// Called when an exception is thrown from the <see cref="AmqpObject.Abort"/> method.
        /// </summary>
        /// <param name="exception">The exception.</param>
        public virtual void AmqpAbortThrowingException(Exception exception)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Critical, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAbortThrowingException(exception.ToString());
            }
        }

        /// <summary>
        /// Called when a message is received and put into the local buffer queue because
        /// there is no receive request and message callback.
        /// </summary>
        /// <param name="link">The receiving link.</param>
        /// <param name="deliveryId">Delivery id of the message.</param>
        /// <param name="transfers">The number of transfer frames received for this message.</param>
        /// <param name="totalLinkCredit">The total credit set on the link.</param>
        /// <param name="linkCredit">The current link credit of the link.</param>
        public virtual void AmqpCacheMessage(AmqpLink link, uint deliveryId, int transfers, uint totalLinkCredit, uint linkCredit)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCacheMessage(link.ToString(), deliveryId, transfers, totalLinkCredit, linkCredit);
            }
        }

        /// <summary>
        /// Called when an IoEvent occurrs.
        /// </summary>
        /// <param name="source">The object that sends the event.</param>
        /// <param name="ioEvent">The event.</param>
        /// <param name="queueSize">The size that the source maintains for such events.</param>
        public virtual void AmqpIoEvent(AmqpObject source, IoEvent ioEvent, long queueSize)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpIoEvent(source.ToString(), (int)ioEvent, queueSize);
            }
        }

#pragma warning disable 1591
        // These APIs are deprecated. Provide there for back compat only. Implementations will
        // not get called but at least they will not get runtime errors.

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpOpenConnection(object source, object connection)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenConnection(source.ToString(), connection.ToString());
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpCloseConnection(object source, object connection, bool abort)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCloseConnection(source.ToString(), connection.ToString(), abort);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpAddSession(object source, object session, ushort localChannel, ushort remoteChannel)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAddSession(source.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpAttachLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAttachLink(connection.ToString(), session.ToString(), link.ToString(), localHandle, remoteHandle, linkName, role, string.Empty);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpDeliveryNotFound(object source, string deliveryTag)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDeliveryNotFound(source.ToString(), deliveryTag);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpDispose(object source, uint deliveryId, bool settled, object state)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDispose(source.ToString(), deliveryId, settled, state == null ? string.Empty : state.ToString());
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpDynamicBufferSizeChange(object source, string type, int oldSize, int newSize)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDynamicBufferSizeChange(source.ToString(), type, oldSize, newSize);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpInsecureTransport(object source, object transport, bool isSecure, bool isAuthenticated)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpInsecureTransport(source.ToString(), transport.ToString(), isSecure, isAuthenticated);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpLinkDetach(object source, string name, uint handle, string action, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLinkDetach(source.ToString(), name, handle, action, error);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpListenSocketAcceptError(object source, bool willRetry, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpListenSocketAcceptError(source.ToString(), willRetry, error);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpLogError(object source, string operation, string message)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogError(source.ToString(), operation, message);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpMissingHandle(object source, string type, uint handle)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Warning, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpMissingHandle(source.ToString(), type, handle);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpOpenEntityFailed(object source, object obj, string name, string entityName, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntityFailed(source.ToString(), name, entityName, error);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpOpenEntitySucceeded(object source, object obj, string name, string entityName)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntitySucceeded(source.ToString(), name, entityName);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpSentMessage(object source, uint deliveryId, long bytes)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSentMessage(source.ToString(), deliveryId, bytes);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpReceiveMessage(object source, uint deliveryId, int transferCount)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpReceiveMessage(source.ToString(), deliveryId, transferCount);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpRemoveLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveLink(connection.ToString(), session.ToString(), link.ToString(), localHandle, remoteHandle, linkName);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpRemoveSession(object source, object session, ushort localChannel, ushort remoteChannel)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveSession(source.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpSessionWindowClosed(object source, int nextId)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSessionWindowClosed(source.ToString(), nextId);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpStateTransition(object source, string operation, object fromState, object toState)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpStateTransition(source.ToString(), operation, fromState.ToString(), toState.ToString());
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpUpgradeTransport(object source, object from, object to)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpUpgradeTransport(source.ToString(), from.ToString(), to.ToString());
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpAbortThrowingException(string exception)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAbortThrowingException(exception);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpCacheMessage(object source, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCacheMessage(source.ToString(), deliveryId, count, totalLinkCredit, linkCredit);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpIoEvent(object source, int ioEvent, long queueSize)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpIoEvent(source.ToString(), ioEvent, queueSize);
            }
        }

        [Obsolete]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void AmqpHandleException(Exception exception, string traceInfo)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpHandleException(exception.ToString(), traceInfo);
            }
        }
#pragma warning restore 1591

        internal static void OnProtocolHeader(ProtocolHeader header, bool send)
        {
            if (FrameLogger != null)
            {
                LogProtocolHeader(header, send);
            }
        }

        internal static void OnFrame(uint id, FrameType type, ushort channel, Performative command, bool send, int frameSize)
        {
            if (FrameLogger != null)
            {
                LogFrame(id, type, channel, command, send, frameSize);
            }
        }

        static void LogProtocolHeader(ProtocolHeader header, bool send)
        {
            StringBuilder sb = new StringBuilder();
            AppendCommon(sb, 0, send);
            sb.Append(' ');
            sb.Append(header);

            FrameLogger(sb.ToString());
        }

        static void LogFrame(uint id, FrameType type, ushort channel, Performative command, bool send, int frameSize)
        {
            StringBuilder sb = new StringBuilder();
            AppendCommon(sb, id, send);
            sb.Append(' ');
            sb.Append("FRM");
            sb.Append('(');
            sb.AppendFormat("{0:X4}", frameSize);
            sb.Append('|');
            sb.AppendFormat("{0:X2}", (int)type);
            sb.AppendFormat("{0:X2}", channel);
            if (command != null)
            {
                sb.Append(' ');
                sb.Append(command);
            }
            sb.Append(')');

            FrameLogger(sb.ToString());
        }

        static void AppendCommon(StringBuilder sb, uint id, bool send)
        {
            sb.Append('[');
            sb.AppendFormat("{0:X4}", id);
            sb.Append(' ');
            sb.AppendFormat("{0:HH:mm:ss.fff}", DateTime.UtcNow);
            sb.Append(']');
            sb.Append(' ');
            sb.Append(send? "SEND" : "RECV");
        }
    }
}
