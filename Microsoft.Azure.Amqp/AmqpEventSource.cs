// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics.Tracing;

    [EventSource(Name = "Microsoft-Azure-Amqp")]
    sealed class AmqpEventSource : EventSource
    {
        public static readonly AmqpEventSource Log;

        static AmqpEventSource()
        {
            Log = new AmqpEventSource();
        }

        [Event(1, Level = EventLevel.Informational, Message = "{0}: open connection {1}.")]
        public void AmqpOpenConnection(string source, string connection)
        {
            WriteEvent(1, source, connection);
        }

        [Event(2, Level = EventLevel.Informational, Message = "{0}: close connection {1} (abort={2}).")]
        public void AmqpCloseConnection(string source, string connection, bool abort)
        {
            WriteEvent(2, source, connection, GetString(abort));
        }

        [Event(3, Level = EventLevel.Informational, Message = "{0}: add session {1} (local={2},remote={3}).")]
        public void AmqpAddSession(string source, string session, ushort localChannel, ushort remoteChannel)
        {
            WriteEvent(3, source, session, localChannel, remoteChannel);
        }

        [Event(4, Level = EventLevel.Informational, Message = "{0}:{1} attach link {2} (local={3},remote={4},name={5},role={6},address={7}).")]
        public void AmqpAttachLink(string connection, string session, string link, uint localHandle, uint remoteHandle, string linkName, string role, string address)
        {
            WriteEvent(4, connection, session, link, localHandle, remoteHandle, linkName, role, address);
        }

        [Event(5, Level = EventLevel.Informational, Message = "{0}: delivery {1} not found.")]
        public void AmqpDeliveryNotFound(string source, string deliveryTag)
        {
            WriteEvent(5, source, deliveryTag);
        }

        [Event(6, Level = EventLevel.Verbose, Message = "{0}: update delivery {1} (settled={2},state={3}).")]
        public void AmqpDispose(string source, uint deliveryId, bool settled, string state)
        {
            WriteEvent(6, source, deliveryId, GetString(settled), state);
        }

        [Event(7, Level = EventLevel.Informational, Message = "{0}: update {1} buffer size (old={2},new={3}).")]
        public void AmqpDynamicBufferSizeChange(string source, string type, int oldSize, int newSize)
        {
            WriteEvent(7, source, type, oldSize, newSize);
        }

        [Event(8, Level = EventLevel.Warning, Message = "{0}: transport {1} is not secure (encrypted={2},authenticated={3}).")]
        public void AmqpInsecureTransport(string source, string transport, bool isSecure, bool isAuthenticated)
        {
            WriteEvent(8, source, transport, GetString(isSecure), GetString(isAuthenticated));
        }

        [Event(9, Level = EventLevel.Informational, Message = "{0}: detach link {1} (handle={2},action={3},error={4}).")]
        public void AmqpLinkDetach(string source, string name, uint handle, string action, string error)
        {
            WriteEvent(9, source, name, handle, action, error);
        }

        [Event(10, Level = EventLevel.Error, Message = "{0}: listen socket error (retry={1},error={2}).")]
        public void AmqpListenSocketAcceptError(string source, bool willRetry, string error)
        {
            WriteEvent(10, source, GetString(willRetry), error);
        }

        [Event(11, Level = EventLevel.Error, Message = "{0}: operation {1} failed with error {2}.")]
        public void AmqpLogError(string source, string operation, string message)
        {
            WriteEvent(11, source, operation, message);
        }

        [Event(12, Level = EventLevel.Informational, Message = "{0}: operation {1} info {2}.")]
        public void AmqpLogOperationInformational(string source, TraceOperation operation, string detail)
        {
            WriteEvent(12, source, operation, detail);
        }

        [Event(13, Level = EventLevel.Verbose, Message = "{0}: operation {1} detail {2}.")]
        public void AmqpLogOperationVerbose(string source, TraceOperation operation, object detail)
        {
            WriteEvent(13, source, operation, detail);
        }

        [Event(14, Level = EventLevel.Warning, Message = "{0}: {1} {2} not found.")]
        public void AmqpMissingHandle(string source, string type, uint handle)
        {
            WriteEvent(14, source, type, handle);
        }

        [Event(15, Level = EventLevel.Error, Message = "{0}: open {1} {2} failed (entity={3},error={4}).")]
        public void AmqpOpenEntityFailed(string source, string obj, string name, string entityName, string error)
        {
            WriteEvent(15, source, obj, name, entityName, error);
        }

        [Event(16, Level = EventLevel.Informational, Message = "{0}: open {1} {2} succeeded (entity={3}).")]
        public void AmqpOpenEntitySucceeded(string source, string obj, string name, string entityName)
        {
            WriteEvent(16, source, obj, name, entityName);
        }

        [Event(17, Level = EventLevel.Verbose, Message = "{0}: received message (id={1},transfers={2}).")]
        public void AmqpReceiveMessage(string source, uint deliveryId, int transferCount)
        {
            WriteEvent(17, source, deliveryId, transferCount);
        }

        [Event(18, Level = EventLevel.Informational, Message = "{0}:{1} remove link {2} (local={3},remote={4},name={5}).")]
        public void AmqpRemoveLink(string connection, string session, string link, uint localHandle, uint remoteHandle, string linkName)
        {
            WriteEvent(18, connection, session, link, localHandle, remoteHandle, linkName);
        }

        [Event(19, Level = EventLevel.Informational, Message = "{0}: remove session {1} (local={2},remote={3}).")]
        public void AmqpRemoveSession(string source, string session, ushort localChannel, ushort remoteChannel)
        {
            WriteEvent(19, source, session, localChannel, remoteChannel);
        }

        [Event(20, Level = EventLevel.Informational, Message = "{0}: session window closed (next={1}).")]
        public void AmqpSessionWindowClosed(string source, int nextId)
        {
            WriteEvent(20, source, nextId);
        }

        [Event(21, Level = EventLevel.Informational, Message = "{0}: change state (operation={1},from={2},to={3}).")]
        public void AmqpStateTransition(string source, string operation, string fromState, string toState)
        {
            WriteEvent(21, source, operation, fromState, toState);
        }

        [Event(22, Level = EventLevel.Informational, Message = "{0}: transport upgrade from {1} to {2}.")]
        public void AmqpUpgradeTransport(string source, string from, string to)
        {
            WriteEvent(22, source, from, to);
        }

        [Event(23, Level = EventLevel.Error, Message = "AmqpObject.Abort threw exception {0}.")]
        public void AmqpAbortThrowingException(string exception)
        {
            WriteEvent(23, exception);
        }

        [Event(24, Level = EventLevel.Verbose, Message = "{0}: cache message (id={1},transfers={2},prefetchBySize={3},cacheSize={4},totalCredit={5},credit={6}).")]
        public void AmqpCacheMessage(string source, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
            WriteEvent(24, source, deliveryId, count, GetString(isPrefecthingBySize), totalCacheSizeInBytes, totalLinkCredit, linkCredit);
        }

        [Event(25, Level = EventLevel.Informational, Message = "{0}: I/O event {1} (queue={2}).")]
        public void AmqpIoEvent(string source, int ioEvent, long queueSize)
        {
            WriteEvent(25, source, ioEvent, queueSize);
        }

        [Event(26, Level = EventLevel.Error, Message = "Handled exception {0} (info={1}).")]
        public void AmqpHandleException(Exception exception, string traceInfo)
        {
            WriteEvent(26, exception.ToStringSlim(), traceInfo);
        }

        static string GetString(bool value)
        {
            return value ? "true" : "false";
        }
    }
}
