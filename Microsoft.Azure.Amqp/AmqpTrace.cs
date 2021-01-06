// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
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
        public static Action<string> TraceCallback { get; set; }

        protected AmqpTrace()
        {
        }

        public virtual void AmqpOpenConnection(object source, object connection)
        {
        }

        public virtual void AmqpCloseConnection(object source, object connection, bool abort)
        {
        }

        public virtual void AmqpAddSession(object source, object session, ushort localChannel, ushort remoteChannel)
        {
        }

        public virtual void AmqpAttachLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
        }

        public virtual void AmqpDeliveryNotFound(object source, string deliveryTag)
        {
        }

        public virtual void AmqpDispose(object source, uint deliveryId, bool settled, object state)
        {
        }

        public virtual void AmqpDynamicBufferSizeChange(object source, string type, int oldSize, int newSize)
        {
        }

        public virtual void AmqpInsecureTransport(object source, object transport, bool isSecure, bool isAuthenticated)
        {
        }

        public virtual void AmqpLinkDetach(object source, string name, uint handle, string action, string error)
        {
        }

        public virtual void AmqpListenSocketAcceptError(object source, bool willRetry, string error)
        {
        }

        public virtual void AmqpLogError(object source, string operation, string message)
        {
        }

        public virtual void AmqpLogOperationInformational(object source, TraceOperation operation, object detail)
        {
        }

        public virtual void AmqpLogOperationVerbose(object source, TraceOperation operation, object detail)
        {
        }

        public virtual void AmqpMissingHandle(object source, string type, uint handle)
        {
        }

        public virtual void AmqpOpenEntityFailed(object source, object obj, string name, string entityName, string error)
        {
        }

        public virtual void AmqpOpenEntitySucceeded(object source, object obj, string name, string entityName)
        {
        }

        public virtual void AmqpReceiveMessage(object source, uint deliveryId, int transferCount)
        {
        }

        public virtual void AmqpRemoveLink(object connection, object session, object link, uint localHandle, uint remoteHandle, string linkName)
        {
        }

        public virtual void AmqpRemoveSession(object source, object session, ushort localChannel, ushort remoteChannel)
        {
        }

        public virtual void AmqpSessionWindowClosed(object source, int nextId)
        {
        }

        public virtual void AmqpStateTransition(object source, string operation, object fromState, object toState)
        {
        }

        public virtual void AmqpUpgradeTransport(object source, object from, object to)
        {
        }

        public virtual void AmqpAbortThrowingException(string exception)
        {
        }

        public virtual void AmqpCacheMessage(object source, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
        }

        public virtual void AmqpIoEvent(object source, int ioEvent, long queueSize)
        {
        }

        public virtual void AmqpHandleException(Exception exception, string traceInfo)
        {
        }

        internal static void AmqpSendReceiveHeader(ProtocolHeader header, bool send)
        {
            if (TraceCallback != null)
            {
                StringBuilder sb = new StringBuilder();
                AppendCommon(sb, send);
                sb.Append(' ');
                sb.Append(header);

                TraceCallback(sb.ToString());
            }
        }

        internal static void AmqpSendReceiveFrame(FrameType type, ushort channel, Performative command, bool send, int frameSize)
        {
            if (TraceCallback != null)
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

                TraceCallback(sb.ToString());
            }
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
