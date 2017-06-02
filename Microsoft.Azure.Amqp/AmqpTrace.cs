// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    class AmqpTrace
    {
        // Replace this with your own implementation to enable tracing.
        public static AmqpTrace Provider = new AmqpTrace();

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
    }
}
