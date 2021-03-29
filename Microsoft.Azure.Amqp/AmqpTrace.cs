// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Text;
    using Microsoft.Azure.Amqp.Framing;

    public class AmqpTrace
    {
        // Replace this with your own implementation to enable tracing.
        public static AmqpTrace Provider = new AmqpTrace();

        /// <summary>
        /// Gets or sets a callback to write library traces. It impacts performance and
        /// should be used for debugging purposes only.
        /// </summary>
        public static Action<string> FrameLogger { get; set; }

        protected AmqpTrace()
        {
        }

        public virtual void AmqpOpenConnection(object source, object connection)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenConnection(source.ToString(), connection.ToString());
            }
        }

        public virtual void AmqpCloseConnection(object source, object connection, bool abort)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCloseConnection(source.ToString(), connection.ToString(), abort);
            }
        }

        public virtual void AmqpAddSession(object source, object session, ushort localChannel, ushort remoteChannel)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAddSession(source.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        public virtual void AmqpAttachLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAttachLink(connection.ToString(), session.ToString(), link.ToString(), localHandle, remoteHandle, linkName, role, string.Empty);
            }
        }

        public virtual void AmqpDeliveryNotFound(object source, string deliveryTag)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDeliveryNotFound(source.ToString(), deliveryTag);
            }
        }

        public virtual void AmqpDispose(object source, uint deliveryId, bool settled, object state)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDispose(source.ToString(), deliveryId, settled, state.ToString());
            }
        }

        public virtual void AmqpDynamicBufferSizeChange(object source, string type, int oldSize, int newSize)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpDynamicBufferSizeChange(source.ToString(), type, oldSize, newSize);
            }
        }

        public virtual void AmqpInsecureTransport(object source, object transport, bool isSecure, bool isAuthenticated)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpInsecureTransport(source.ToString(), transport.ToString(), isSecure, isAuthenticated);
            }
        }

        public virtual void AmqpLinkDetach(object source, string name, uint handle, string action, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLinkDetach(source.ToString(), name, handle, action, error);
            }
        }

        public virtual void AmqpListenSocketAcceptError(object source, bool willRetry, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpListenSocketAcceptError(source.ToString(), willRetry, error);
            }
        }

        public virtual void AmqpLogError(object source, string operation, string message)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogError(source.ToString(), operation, message);
            }
        }

        public virtual void AmqpLogOperationInformational(object source, TraceOperation operation, object detail)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogOperationInformational(source.ToString(), operation, detail.ToString());
            }
        }

        public virtual void AmqpLogOperationVerbose(object source, TraceOperation operation, object detail)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpLogOperationVerbose(source.ToString(), operation, detail.ToString());
            }
        }

        public virtual void AmqpMissingHandle(object source, string type, uint handle)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Warning, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpMissingHandle(source.ToString(), type, handle);
            }
        }

        public virtual void AmqpOpenEntityFailed(object source, object obj, string name, string entityName, string error)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntityFailed(source.ToString(), obj.ToString(), name, entityName, error);
            }
        }

        public virtual void AmqpOpenEntitySucceeded(object source, object obj, string name, string entityName)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpOpenEntitySucceeded(source.ToString(), obj.ToString(), name, entityName);
            }
        }

        public virtual void AmqpReceiveMessage(object source, uint deliveryId, int transferCount)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpReceiveMessage(source.ToString(), deliveryId, transferCount);
            }
        }

        public virtual void AmqpRemoveLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveLink(connection.ToString(), session.ToString(), link.ToString(), localHandle, remoteHandle, linkName);
            }
        }

        public virtual void AmqpRemoveSession(object source, object session, ushort localChannel, ushort remoteChannel)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpRemoveSession(source.ToString(), session.ToString(), localChannel, remoteChannel);
            }
        }

        public virtual void AmqpSessionWindowClosed(object source, int nextId)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpSessionWindowClosed(source.ToString(), nextId);
            }
        }

        public virtual void AmqpStateTransition(object source, string operation, object fromState, object toState)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpStateTransition(source.ToString(), operation, fromState.ToString(), toState.ToString());
            }
        }

        public virtual void AmqpUpgradeTransport(object source, object from, object to)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpUpgradeTransport(source.ToString(), from.ToString(), to.ToString());
            }
        }

        public virtual void AmqpAbortThrowingException(string exception)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpAbortThrowingException(exception);
            }
        }

        public virtual void AmqpCacheMessage(object source, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Verbose, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpCacheMessage(source.ToString(), deliveryId, count, isPrefecthingBySize, totalCacheSizeInBytes, totalLinkCredit, linkCredit);
            }
        }

        public virtual void AmqpIoEvent(object source, int ioEvent, long queueSize)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Informational, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpIoEvent(source.ToString(), ioEvent, queueSize);
            }
        }

        public virtual void AmqpHandleException(Exception exception, string traceInfo)
        {
            if (AmqpEventSource.Log.IsEnabled(EventLevel.Error, EventKeywords.None))
            {
                AmqpEventSource.Log.AmqpHandleException(exception, traceInfo);
            }
        }

        internal static void AmqpSendReceiveHeader(ProtocolHeader header, bool send)
        {
            if (FrameLogger != null)
            {
                TraceHeader(header, send);
            }
        }

        internal static void AmqpSendReceiveFrame(FrameType type, ushort channel, Performative command, bool send, int frameSize)
        {
            if (FrameLogger != null)
            {
                TraceFrame(type, channel, command, send, frameSize);
            }
        }

        static void TraceHeader(ProtocolHeader header, bool send)
        {
            StringBuilder sb = new StringBuilder();
            AppendCommon(sb, send);
            sb.Append(' ');
            sb.Append(header);
            FrameLogger(sb.ToString());
        }

        static void TraceFrame(FrameType type, ushort channel, Performative command, bool send, int frameSize)
        {
            StringBuilder sb = new StringBuilder();
            AppendCommon(sb, send);
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

        static void AppendCommon(StringBuilder sb, bool send)
        {
            sb.Append('[');
            sb.AppendFormat("{0:X3}", Diagnostics.CurrentProcess.ID);
            sb.Append('.');
            sb.AppendFormat("{0:X3}", Environment.CurrentManagedThreadId);
            sb.Append(' ');
            sb.AppendFormat("{0:HH:mm:ss.fff}", DateTime.UtcNow);
            sb.Append(']');
            sb.Append(' ');
            sb.Append(send ? "SEND" : "RECV");
        }
    }
}
