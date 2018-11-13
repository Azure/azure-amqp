// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Transport;

    public class AmqpTrace
    {
        // Replace this with your own implementation to enable tracing.
        public static AmqpTrace Provider = new AmqpTrace();

        protected AmqpTrace()
        {
        }

        public virtual void AmqpOpenConnection(AmqpConnection connection)
        {
        }

        public virtual void AmqpCloseConnection(AmqpConnection connection, bool abort)
        {
        }

        public virtual void AmqpAddSession(AmqpConnection connection, AmqpSession session, ushort localChannel, ushort remoteChannel)
        {
        }

        public virtual void AmqpAttachLink(AmqpSession session, AmqpLink link, uint localHandle, uint remoteHandle, string linkName, string role, object source, object target)
        {
        }

        public virtual void AmqpDeliveryNotFound(AmqpLink link, string deliveryTag)
        {
        }

        public virtual void AmqpDispose(AmqpLink link, uint deliveryId, bool settled, object state)
        {
        }

        public virtual void AmqpDynamicBufferSizeChange(TransportBase transport, string type, int oldSize, int newSize)
        {
        }

        public virtual void AmqpInsecureTransport(AmqpTransportListener listener, TransportBase transport, bool isSecure, bool isAuthenticated)
        {
        }

        public virtual void AmqpLinkDetach(AmqpLink link, string name, uint handle, string action, string error)
        {
        }

        public virtual void AmqpListenSocketAcceptError(TransportListener listener, bool willRetry, string error)
        {
        }

        public virtual void AmqpLogError(object source, string operation, Exception exception)
        {
        }

        public virtual void AmqpLogOperationInformational(object source, TraceOperation operation, object detail)
        {
        }

        public virtual void AmqpLogOperationVerbose(object source, TraceOperation operation, object detail)
        {
        }

        public virtual void AmqpMissingHandle(AmqpConnection connection, AmqpSession session, string type, uint handle)
        {
        }

        public virtual void AmqpOpenEntityFailed(object source, string name, string entityName, Exception error)
        {
        }

        public virtual void AmqpOpenEntitySucceeded(object source, string name, string entityName)
        {
        }

        public virtual void AmqpReceiveMessage(AmqpLink link, uint deliveryId, int transferCount)
        {
        }

        public virtual void AmqpRemoveLink(AmqpSession session, object link, uint localHandle, uint remoteHandle, string linkName)
        {
        }

        public virtual void AmqpRemoveSession(AmqpConnection connection, AmqpSession session, ushort localChannel, ushort remoteChannel)
        {
        }

        public virtual void AmqpSessionWindowClosed(AmqpSession session, int nextId)
        {
        }

        public virtual void AmqpStateTransition(AmqpObject source, string operation, object fromState, object toState)
        {
        }

        public virtual void AmqpUpgradeTransport(object source, TransportBase from, TransportBase to)
        {
        }

        public virtual void AmqpAbortThrowingException(Exception exception)
        {
        }

        public virtual void AmqpCacheMessage(AmqpLink link, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
        }

        public virtual void AmqpIoEvent(AmqpObject source, int ioEvent, long queueSize)
        {
        }
    }
}
