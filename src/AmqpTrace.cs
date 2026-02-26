// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// Provides trace/logging callbacks for AMQP operations. Set <see cref="Provider"/> to a custom subclass to override.
    /// </summary>
    public class AmqpTrace
    {
        /// <summary>
        /// Set this to a custom AmqpTrace subclass to override trace behavior.
        /// </summary>
        public static AmqpTrace Provider;

        /// <summary>
        /// Gets or sets a callback to write library traces. It impacts performance and
        /// should be used for debugging purposes only.
        /// </summary>
        public static Action<string> FrameLogger { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="AmqpTrace"/> class.
        /// </summary>
        protected AmqpTrace()
        {
        }

        #region Virtual Methods (shipped public API)

        /// <summary>Traces an AMQP connection open.</summary>
        public virtual void AmqpOpenConnection(object source, object connection) { }

        /// <summary>Traces an AMQP connection close.</summary>
        public virtual void AmqpCloseConnection(object source, object connection, bool abort) { }

        /// <summary>Traces an AMQP session add.</summary>
        public virtual void AmqpAddSession(object source, object session, ushort localChannel, ushort remoteChannel) { }

        /// <summary>Traces an AMQP link attach.</summary>
        public virtual void AmqpAttachLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName, string role, object source, object target) { }

        /// <summary>Traces a delivery not found event.</summary>
        public virtual void AmqpDeliveryNotFound(object source, string deliveryTag) { }

        /// <summary>Traces an AMQP disposition.</summary>
        public virtual void AmqpDispose(object source, uint deliveryId, bool settled, object state) { }

        /// <summary>Traces a dynamic buffer size change.</summary>
        public virtual void AmqpDynamicBufferSizeChange(object source, string type, int oldSize, int newSize) { }

        /// <summary>Traces an insecure transport event.</summary>
        public virtual void AmqpInsecureTransport(object source, object transport, bool isSecure, bool isAuthenticated) { }

        /// <summary>Traces an AMQP link detach.</summary>
        public virtual void AmqpLinkDetach(object source, string name, uint handle, string action, string error) { }

        /// <summary>Traces a listen socket accept error.</summary>
        public virtual void AmqpListenSocketAcceptError(object source, bool willRetry, string error) { }

        /// <summary>Traces an AMQP error.</summary>
        public virtual void AmqpLogError(object source, string operation, string message) { }

        /// <summary>Traces an informational AMQP operation.</summary>
        public virtual void AmqpLogOperationInformational(object source, TraceOperation operation, object detail) { }

        /// <summary>Traces a verbose AMQP operation.</summary>
        public virtual void AmqpLogOperationVerbose(object source, TraceOperation operation, object detail) { }

        /// <summary>Traces a missing handle event.</summary>
        public virtual void AmqpMissingHandle(object source, string type, uint handle) { }

        /// <summary>Traces an entity open failure.</summary>
        public virtual void AmqpOpenEntityFailed(object source, object obj, string name, string entityName, string error) { }

        /// <summary>Traces an entity open success.</summary>
        public virtual void AmqpOpenEntitySucceeded(object source, object obj, string name, string entityName) { }

        /// <summary>Traces a sent message.</summary>
        public virtual void AmqpSentMessage(object source, uint deliveryId, long bytes) { }

        /// <summary>Traces a received message.</summary>
        public virtual void AmqpReceiveMessage(object source, uint deliveryId, int transferCount) { }

        /// <summary>Traces a link removal.</summary>
        public virtual void AmqpRemoveLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName) { }

        /// <summary>Traces a session removal.</summary>
        public virtual void AmqpRemoveSession(object source, object session, ushort localChannel, ushort remoteChannel) { }

        /// <summary>Traces a session window closed event.</summary>
        public virtual void AmqpSessionWindowClosed(object source, int nextId) { }

        /// <summary>Traces an AMQP state transition.</summary>
        public virtual void AmqpStateTransition(object source, string operation, object fromState, object toState) { }

        /// <summary>Traces a transport upgrade.</summary>
        public virtual void AmqpUpgradeTransport(object source, object from, object to) { }

        /// <summary>Traces an exception thrown during abort.</summary>
        public virtual void AmqpAbortThrowingException(string exception) { }

        /// <summary>Traces a cached message event.</summary>
        public virtual void AmqpCacheMessage(object source, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit) { }

        /// <summary>Traces an IO event.</summary>
        public virtual void AmqpIoEvent(object source, int ioEvent, long queueSize) { }

        /// <summary>Traces a handled exception.</summary>
        public virtual void AmqpHandleException(Exception exception, string traceInfo) { }

        #endregion

        #region Internal Static Methods (strongly-typed for internal callers)

        internal static void OnOpenConnection(object source, AmqpConnection connection)
        {
            if (Provider != null)
            {
                Provider.AmqpOpenConnection(source, connection);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenConnection(source.ToString(), connection.ToString());
            }
        }

        internal static void OnCloseConnection(object source, AmqpConnection connection, bool abort)
        {
            if (Provider != null)
            {
                Provider.AmqpCloseConnection(source, connection, abort);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCloseConnection(source.ToString(), connection.ToString(), abort);
            }
        }

        internal static void OnAddSession(AmqpConnection connection, AmqpSession session, ushort localChannel, ushort remoteChannel)
        {
            if (Provider != null)
            {
                Provider.AmqpAddSession(connection, session, localChannel, remoteChannel);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAddSession(connection.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        internal static void OnAttachLink(AmqpConnection connection, AmqpSession session, AmqpLink link,
            uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
            if (Provider != null)
            {
                Provider.AmqpAttachLink(connection, session, link, localHandle, remoteHandle, linkName, role, source, target);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAttachLink(connection.ToString(), session.ToString(), link.ToString(),
                    localHandle, remoteHandle, linkName, role, string.Empty);
            }
        }

        internal static void OnDeliveryNotFound(AmqpLink link, string deliveryTag)
        {
            if (Provider != null)
            {
                Provider.AmqpDeliveryNotFound(link, deliveryTag);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDeliveryNotFound(link.ToString(), deliveryTag);
            }
        }

        internal static void OnDispose(AmqpLink link, uint deliveryId, bool settled, object state)
        {
            if (Provider != null)
            {
                Provider.AmqpDispose(link, deliveryId, settled, state);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDispose(link.ToString(), deliveryId, settled, state == null ? string.Empty : state.ToString());
            }
        }

        internal static void OnDynamicBufferSizeChange(TransportBase transport, string type, int oldSize, int newSize)
        {
            if (Provider != null)
            {
                Provider.AmqpDynamicBufferSizeChange(transport, type, oldSize, newSize);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDynamicBufferSizeChange(transport.ToString(), type, oldSize, newSize);
            }
        }

        internal static void OnInsecureTransport(AmqpTransportListener listener, TransportBase transport, bool isSecure, bool isAuthenticated)
        {
            if (Provider != null)
            {
                Provider.AmqpInsecureTransport(listener, transport, isSecure, isAuthenticated);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpInsecureTransport(listener.ToString(), transport.ToString(), isSecure, isAuthenticated);
            }
        }

        internal static void OnLinkDetach(AmqpLink link, string name, uint handle, string action, string error)
        {
            if (Provider != null)
            {
                Provider.AmqpLinkDetach(link, name, handle, action, error);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLinkDetach(link.ToString(), name, handle, action, error);
            }
        }

        internal static void OnListenSocketAcceptError(TransportListener listener, bool willRetry, string error)
        {
            if (Provider != null)
            {
                Provider.AmqpListenSocketAcceptError(listener, willRetry, error);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpListenSocketAcceptError(listener.ToString(), willRetry, error);
            }
        }

        internal static void OnLogError(object source, string operation, Exception exception)
        {
            if (Provider != null)
            {
                Provider.AmqpLogError(source, operation, exception.ToString());
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogError(source.ToString(), operation, exception.ToString());
            }
        }

        internal static void OnLogOperationInformational(object source, TraceOperation operation, object detail)
        {
            if (Provider != null)
            {
                Provider.AmqpLogOperationInformational(source, operation, detail);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogOperationInformational(source.ToString(), operation, detail.ToString());
            }
        }

        internal static void OnLogOperationVerbose(object source, TraceOperation operation, object detail)
        {
            if (Provider != null)
            {
                Provider.AmqpLogOperationVerbose(source, operation, detail);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogOperationVerbose(source.ToString(), operation, detail.ToString());
            }
        }

        internal static void OnMissingHandle(AmqpObject container, string type, uint handle)
        {
            if (Provider != null)
            {
                Provider.AmqpMissingHandle(container, type, handle);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Warning, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpMissingHandle(container.ToString(), type, handle);
            }
        }

        internal static void OnOpenEntityFailed(object source, string name, string entityName, Exception error)
        {
            if (Provider != null)
            {
                Provider.AmqpOpenEntityFailed(source, source, name, entityName, error.ToString());
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntityFailed(source.ToString(), name, entityName, error.ToString());
            }
        }

        internal static void OnOpenEntitySucceeded(object source, string name, string entityName)
        {
            if (Provider != null)
            {
                Provider.AmqpOpenEntitySucceeded(source, source, name, entityName);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntitySucceeded(source.ToString(), name, entityName);
            }
        }

        internal static void OnSentMessage(AmqpLink link, uint deliveryId, long bytes)
        {
            if (Provider != null)
            {
                Provider.AmqpSentMessage(link, deliveryId, bytes);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSentMessage(link.ToString(), deliveryId, bytes);
            }
        }

        internal static void OnReceiveMessage(AmqpLink link, uint deliveryId, int transferCount)
        {
            if (Provider != null)
            {
                Provider.AmqpReceiveMessage(link, deliveryId, transferCount);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpReceiveMessage(link.ToString(), deliveryId, transferCount);
            }
        }

        internal static void OnRemoveLink(AmqpSession session, object link, uint localHandle, uint remoteHandle, string linkName)
        {
            if (Provider != null)
            {
                Provider.AmqpRemoveLink(session.Connection, session, link, localHandle, remoteHandle, linkName);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveLink(session.Connection.ToString(), session.ToString(), link.ToString(), localHandle, remoteHandle, linkName);
            }
        }

        internal static void OnRemoveSession(AmqpConnection connection, AmqpSession session, ushort localChannel, ushort remoteChannel)
        {
            if (Provider != null)
            {
                Provider.AmqpRemoveSession(connection, session, localChannel, remoteChannel);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveSession(connection.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        internal static void OnSessionWindowClosed(AmqpSession session, int nextId)
        {
            if (Provider != null)
            {
                Provider.AmqpSessionWindowClosed(session, nextId);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSessionWindowClosed(session.ToString(), nextId);
            }
        }

        internal static void OnStateTransition(AmqpObject source, string operation, AmqpObjectState fromState, AmqpObjectState toState)
        {
            if (Provider != null)
            {
                Provider.AmqpStateTransition(source, operation, fromState, toState);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpStateTransition(source.ToString(), operation, fromState.ToString(), toState.ToString());
            }
        }

        internal static void OnUpgradeTransport(object source, TransportBase from, TransportBase to)
        {
            if (Provider != null)
            {
                Provider.AmqpUpgradeTransport(source, from, to);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpUpgradeTransport(source.ToString(), from.ToString(), to.ToString());
            }
        }

        internal static void OnAbortThrowingException(Exception exception)
        {
            if (Provider != null)
            {
                Provider.AmqpAbortThrowingException(exception.ToString());
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAbortThrowingException(exception.ToString());
            }
        }

        internal static void OnCacheMessage(AmqpLink link, uint deliveryId, int transfers, uint totalLinkCredit, uint linkCredit)
        {
            if (Provider != null)
            {
                Provider.AmqpCacheMessage(link, deliveryId, transfers, false, 0, totalLinkCredit, linkCredit);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCacheMessage(link.ToString(), deliveryId, transfers, totalLinkCredit, linkCredit);
            }
        }

        internal static void OnIoEvent(AmqpObject source, IoEvent ioEvent, long queueSize)
        {
            if (Provider != null)
            {
                Provider.AmqpIoEvent(source, (int)ioEvent, queueSize);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpIoEvent(source.ToString(), (int)ioEvent, queueSize);
            }
        }

        internal static void OnHandleException(Exception exception, string traceInfo)
        {
            if (Provider != null)
            {
                Provider.AmqpHandleException(exception, traceInfo);
            }
            else if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpHandleException(exception.ToString(), traceInfo);
            }
        }

        #endregion

        #region Frame Logging

        /// <summary>
        /// A callback that is invoked when a Frame is received on a Connection.
        /// </summary>
        internal static Action<uint, Performative> ReceivedFrames;

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

            if (!send)
            {
                ReceivedFrames?.Invoke(id, command);
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
            sb.Append(send ? "SEND" : "RECV");
        }

        #endregion
    }
}
