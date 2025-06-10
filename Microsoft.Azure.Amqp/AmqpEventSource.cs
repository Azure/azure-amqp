// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics.Tracing;

    [EventSource(Name = "Microsoft-Azure-Amqp")]
    sealed class AmqpEventSource : EventSource
    {
        // 1. Caller should check IsEnabled before calling the event methods.
        // 2. Avoid calling the WriteEvent(int eventId, params object[] args) overload
        //    to reduce allocation and type lookup when everything is known.
        // 3. When changing the event definition, update callers in AmqpTrace
        //    if needed.

        private const string EventSourceSuppressMessage = "Parameters to this method are primitive and are trimmer safe.";
        public static readonly AmqpEventSource Log;

        static AmqpEventSource()
        {
            Log = new AmqpEventSource();
        }

        [Event(1, Level = EventLevel.Informational, Message = "{0}: open connection {1}.")]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        public unsafe void AmqpOpenConnection(string source, string connection)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrConnection = connection)
            {
                WriteEvent(
                    1,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrConnection, (connection.Length + 1) * 2
                );
            }
        }

        [Event(2, Level = EventLevel.Informational, Message = "{0}: close connection {1} (abort={2}).")]
        public unsafe void AmqpCloseConnection(string source, string connection, bool abort)
        {
            int b = abort ? 1 : 0;
            fixed (char* ptrSource = source)
            fixed (char* ptrConnection = connection)
            {
                WriteEvent(
                    2,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrConnection, (connection.Length + 1) * 2,
                    (IntPtr)(&b), 4
                );
            }
        }

        [Event(3, Level = EventLevel.Informational, Message = "{0}: add session {1} (local={2},remote={3}).")]
        public unsafe void AmqpAddSession(string source, string session, ushort localChannel, ushort remoteChannel)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrSession = session)
            {
                WriteEvent(
                    3,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrSession, (session.Length + 1) * 2,
                    (IntPtr)(&localChannel), 2,
                    (IntPtr)(&remoteChannel), 2);
            }
        }

        [Event(4, Level = EventLevel.Informational, Message = "{0}:{1} attach link {2} (local={3},remote={4},name={5},role={6},address={7}).")]
        public unsafe void AmqpAttachLink(string connection, string session, string link, uint localHandle, uint remoteHandle, string linkName, string role, string address)
        {
            fixed (char* ptrConnection = connection)
            fixed (char* ptrSession = session)
            fixed (char* ptrLink = link)
            fixed (char* ptrLinkName = linkName)
            fixed (char* ptrRole = role)
            fixed (char* ptrAddress = address)
            {
                WriteEvent(
                    4,  // eventId
                    (IntPtr)ptrConnection, (connection.Length + 1) * 2,
                    (IntPtr)ptrSession, (session.Length + 1) * 2,
                    (IntPtr)ptrLink, (link.Length + 1) * 2,
                    (IntPtr)(&localHandle), 4,
                    (IntPtr)(&remoteHandle), 4,
                    (IntPtr)ptrLinkName, (linkName.Length + 1) * 2,
                    (IntPtr)ptrRole, (role.Length + 1) * 2,
                    (IntPtr)ptrAddress, (address.Length + 1) * 2
                );
            }
        }

        [Event(5, Level = EventLevel.Informational, Message = "{0}: delivery {1} not found.")]
        public unsafe void AmqpDeliveryNotFound(string source, string deliveryTag)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrTag = deliveryTag)
            {
                WriteEvent(
                    5,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrTag, (deliveryTag.Length + 1) * 2
                );
            }
        }

        [Event(6, Level = EventLevel.Verbose, Message = "{0}: update delivery {1} (settled={2},state={3}).")]
        public unsafe void AmqpDispose(string source, uint deliveryId, bool settled, string state)
        {
            int b = settled ? 1 : 0;
            fixed (char* ptrSource = source)
            fixed (char* ptrState = state)
            {
                WriteEvent(
                    6,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&deliveryId), 4,
                    (IntPtr)(&b), 4,
                    (IntPtr)ptrState, (state.Length + 1) * 2
                );
            }
        }

        [Event(7, Level = EventLevel.Informational, Message = "{0}: update {1} buffer size (old={2},new={3}).")]
        public unsafe void AmqpDynamicBufferSizeChange(string source, string type, int oldSize, int newSize)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrType = type)
            {
                WriteEvent(
                    7,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrType, (type.Length + 1) * 2,
                    (IntPtr)(&oldSize), 4,
                    (IntPtr)(&newSize), 4
                );
            }
        }

        [Event(8, Level = EventLevel.Warning, Message = "{0}: transport {1} is not secure (encrypted={2},authenticated={3}).")]
        public unsafe void AmqpInsecureTransport(string source, string transport, bool isSecure, bool isAuthenticated)
        {
            int b1 = isSecure ? 1 : 0;
            int b2 = isAuthenticated ? 1 : 0;
            fixed (char* ptrSource = source)
            fixed (char* ptrTransport = transport)
            {
                WriteEvent(
                    8,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrTransport, (transport.Length + 1) * 2,
                    (IntPtr)(&b1), 4,
                    (IntPtr)(&b2), 4
                );
            }
        }

        [Event(9, Level = EventLevel.Informational, Message = "{0}: detach link {1} (handle={2},action={3},error={4}).")]
        public unsafe void AmqpLinkDetach(string source, string name, uint handle, string action, string error)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrName = name)
            fixed (char* ptrAction = action)
            fixed (char* ptrError = error)
            {
                WriteEvent(
                    9,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrName, (name.Length + 1) * 2,
                    (IntPtr)(&handle), 4,
                    (IntPtr)ptrAction, (action.Length + 1) * 2,
                    (IntPtr)ptrError, (error.Length + 1) * 2
                );
            }
        }

        [Event(10, Level = EventLevel.Error, Message = "{0}: listen socket error (retry={1},error={2}).")]
        public unsafe void AmqpListenSocketAcceptError(string source, bool willRetry, string error)
        {
            int b = willRetry ? 1 : 0;
            fixed (char* ptrSource = source)
            fixed (char* ptrError = error)
            {
                WriteEvent(
                    10,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&b), 4,
                    (IntPtr)ptrError, (error.Length + 1) * 2
                );
            }
        }

        [Event(11, Level = EventLevel.Error, Message = "{0}: operation {1} failed with error {2}.")]
        public unsafe void AmqpLogError(string source, string operation, string message)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrOperation = operation)
            fixed (char* ptrMessage = message)
            {
                WriteEvent(
                    11,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrOperation, (operation.Length + 1) * 2,
                    (IntPtr)ptrMessage, (message.Length + 1) * 2
                );
            }
        }

        [Event(12, Level = EventLevel.Informational, Message = "{0}: operation {1} info {2}.")]
        public unsafe void AmqpLogOperationInformational(string source, TraceOperation operation, string detail)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrDetail = detail)
            {
                WriteEvent(
                    12,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&operation), 4,
                    (IntPtr)ptrDetail, (detail.Length + 1) * 2
                );
            }
        }

        [Event(13, Level = EventLevel.Verbose, Message = "{0}: operation {1} detail {2}.")]
        public unsafe void AmqpLogOperationVerbose(string source, TraceOperation operation, string detail)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrDetail = detail)
            {
                WriteEvent(
                    13,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&operation), 4,
                    (IntPtr)ptrDetail, (detail.Length + 1) * 2
                );
            }
        }

        [Event(14, Level = EventLevel.Warning, Message = "{0}: {1} {2} not found.")]
        public unsafe void AmqpMissingHandle(string source, string type, uint handle)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrType = type)
            {
                WriteEvent(
                    14,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrType, (type.Length + 1) * 2,
                    (IntPtr)(&handle), 4
                );
            }
        }

        [Event(15, Level = EventLevel.Error, Message = "{0}: open {1} {2} failed (entity={3},error={4}).")]
        public unsafe void AmqpOpenEntityFailed(string source, string obj, string name, string entityName, string error)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrObj = obj)
            fixed (char* ptrName = name)
            fixed (char* ptrEntityName = entityName)
            fixed (char* ptrError = error)
            {
                WriteEvent(
                    15,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrObj, (obj.Length + 1) * 2,
                    (IntPtr)ptrName, (name.Length + 1) * 2,
                    (IntPtr)ptrEntityName, (entityName.Length + 1) * 2,
                    (IntPtr)ptrError, (error.Length + 1) * 2
                );
            }
        }

        [Event(16, Level = EventLevel.Informational, Message = "{0}: open {1} {2} succeeded (entity={3}).")]
        public unsafe void AmqpOpenEntitySucceeded(string source, string obj, string name, string entityName)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrObj = obj)
            fixed (char* ptrName = name)
            fixed (char* ptrEntityName = entityName)
            {
                WriteEvent(
                    16,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrObj, (obj.Length + 1) * 2,
                    (IntPtr)ptrName, (name.Length + 1) * 2,
                    (IntPtr)ptrEntityName, (entityName.Length + 1) * 2
                );
            }
        }

        [Event(17, Level = EventLevel.Verbose, Message = "{0}: received message (id={1},transfers={2}).")]
        public unsafe void AmqpReceiveMessage(string source, uint deliveryId, int transferCount)
        {
            fixed (char* ptrSource = source)
            {
                WriteEvent(
                    17,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&deliveryId), 4,
                    (IntPtr)(&transferCount), 4
                );
            }
        }

        [Event(18, Level = EventLevel.Informational, Message = "{0}:{1} remove link {2} (local={3},remote={4},name={5}).")]
        public unsafe void AmqpRemoveLink(string connection, string session, string link, uint localHandle, uint remoteHandle, string linkName)
        {
            fixed (char* ptrConnection = connection)
            fixed (char* ptrSession = session)
            fixed (char* ptrLink = link)
            fixed (char* ptrLinkName = linkName)
            {
                WriteEvent(
                    18,  // eventId
                    (IntPtr)ptrConnection, (connection.Length + 1) * 2,
                    (IntPtr)ptrSession, (session.Length + 1) * 2,
                    (IntPtr)ptrLink, (link.Length + 1) * 2,
                    (IntPtr)(&localHandle), 4,
                    (IntPtr)(&remoteHandle), 4,
                    (IntPtr)ptrLinkName, (linkName.Length + 1) * 2
                );
            }
        }

        [Event(19, Level = EventLevel.Informational, Message = "{0}: remove session {1} (local={2},remote={3}).")]
        public unsafe void AmqpRemoveSession(string connection, string session, ushort localChannel, ushort remoteChannel)
        {
            fixed (char* ptrConnection = connection)
            fixed (char* ptrSession = session)
            {
                WriteEvent(
                    19,  // eventId
                    (IntPtr)ptrConnection, (connection.Length + 1) * 2,
                    (IntPtr)ptrSession, (session.Length + 1) * 2,
                    (IntPtr)(&localChannel), 4,
                    (IntPtr)(&remoteChannel), 4
                );
            }
        }

        [Event(20, Level = EventLevel.Informational, Message = "{0}: session window closed (next={1}).")]
        public unsafe void AmqpSessionWindowClosed(string source, int nextId)
        {
            fixed (char* ptrSource = source)
            {
                WriteEvent(
                    20,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&nextId), 4
                );
            }
        }

        [Event(21, Level = EventLevel.Informational, Message = "{0}: change state (operation={1},from={2},to={3}).")]
        public unsafe void AmqpStateTransition(string source, string operation, string fromState, string toState)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrOperation = operation)
            fixed (char* ptrFromState = fromState)
            fixed (char* ptrToState = toState)
            {
                WriteEvent(
                    21,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrOperation, (operation.Length + 1) * 2,
                    (IntPtr)ptrFromState, (fromState.Length + 1) * 2,
                    (IntPtr)ptrToState, (toState.Length + 1) * 2
                );
            }
        }

        [Event(22, Level = EventLevel.Informational, Message = "{0}: transport upgrade from {1} to {2}.")]
        public unsafe void AmqpUpgradeTransport(string source, string from, string to)
        {
            fixed (char* ptrSource = source)
            fixed (char* ptrFrom = from)
            fixed (char* ptrTo = to)
            {
                WriteEvent(
                    22,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)ptrFrom, (from.Length + 1) * 2,
                    (IntPtr)ptrTo, (to.Length + 1) * 2
                );
            }
        }

        [Event(23, Level = EventLevel.Error, Message = "AmqpObject.Abort threw exception {0}.")]
        public unsafe void AmqpAbortThrowingException(string exception)
        {
            WriteEvent(23, exception);
        }

        [Event(24, Level = EventLevel.Verbose, Message = "{0}: cache message (id={1},transfers={2},prefetchBySize={3},cacheSize={4},totalCredit={5},credit={6}).")]
        public unsafe void AmqpCacheMessage(string source, uint deliveryId, int count, bool isPrefecthingBySize, long totalCacheSizeInBytes, uint totalLinkCredit, uint linkCredit)
        {
            int b = isPrefecthingBySize ? 1 : 0;
            fixed (char* ptrSource = source)
            {
                WriteEvent(
                    24,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&deliveryId), 4,
                    (IntPtr)(&count), 4,
                    (IntPtr)(&b), 4,
                    (IntPtr)(&totalCacheSizeInBytes), 8,
                    (IntPtr)(&totalLinkCredit), 4,
                    (IntPtr)(&linkCredit), 4
                );
            }
        }

        [Event(25, Level = EventLevel.Informational, Message = "{0}: I/O event {1} (queue={2}).")]
        public unsafe void AmqpIoEvent(string source, int ioEvent, long queueSize)
        {
            fixed (char* ptrSource = source)
            {
                WriteEvent(
                    25,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&ioEvent), 4,
                    (IntPtr)(&queueSize), 8
                );
            }
        }

        [Event(26, Level = EventLevel.Error, Message = "Handled exception {0} (info={1}).")]
        public void AmqpHandleException(string exception, string traceInfo)
        {
            WriteEvent(26, exception, traceInfo);
        }

        [Event(27, Level = EventLevel.Verbose, Message = "{0}: sent message (id={1},bytes={2}).")]
        public unsafe void AmqpSentMessage(string source, uint deliveryId, long bytes)
        {
            fixed (char* ptrSource = source)
            {
                WriteEvent(
                    27,  // eventId
                    (IntPtr)ptrSource, (source.Length + 1) * 2,
                    (IntPtr)(&deliveryId), 4,
                    (IntPtr)(&bytes), 8
                );
            }
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[2];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            WriteEventCore(eventId, 2, descrs);
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2, IntPtr a3, int size3)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[3];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            descrs[2].DataPointer = a3;
            descrs[2].Size = size3;
            WriteEventCore(eventId, 3, descrs);
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2,
            IntPtr a3, int size3, IntPtr a4, int size4)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[4];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            descrs[2].DataPointer = a3;
            descrs[2].Size = size3;
            descrs[3].DataPointer = a4;
            descrs[3].Size = size4;
            WriteEventCore(eventId, 4, descrs);
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2,
            IntPtr a3, int size3, IntPtr a4, int size4, IntPtr a5, int size5)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[5];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            descrs[2].DataPointer = a3;
            descrs[2].Size = size3;
            descrs[3].DataPointer = a4;
            descrs[3].Size = size4;
            descrs[4].DataPointer = a5;
            descrs[4].Size = size5;
            WriteEventCore(eventId, 5, descrs);
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2,
            IntPtr a3, int size3, IntPtr a4, int size4, IntPtr a5, int size5, IntPtr a6, int size6)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[6];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            descrs[2].DataPointer = a3;
            descrs[2].Size = size3;
            descrs[3].DataPointer = a4;
            descrs[3].Size = size4;
            descrs[4].DataPointer = a5;
            descrs[4].Size = size5;
            descrs[5].DataPointer = a6;
            descrs[5].Size = size6;
            WriteEventCore(eventId, 6, descrs);
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2,
            IntPtr a3, int size3, IntPtr a4, int size4, IntPtr a5, int size5, IntPtr a6, int size6,
            IntPtr a7, int size7)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[7];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            descrs[2].DataPointer = a3;
            descrs[2].Size = size3;
            descrs[3].DataPointer = a4;
            descrs[3].Size = size4;
            descrs[4].DataPointer = a5;
            descrs[4].Size = size5;
            descrs[5].DataPointer = a6;
            descrs[5].Size = size6;
            descrs[6].DataPointer = a7;
            descrs[6].Size = size7;
            WriteEventCore(eventId, 7, descrs);
        }

        [NonEvent]
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026", Justification = EventSourceSuppressMessage)]
#endif
        unsafe void WriteEvent(int eventId, IntPtr a1, int size1, IntPtr a2, int size2,
            IntPtr a3, int size3, IntPtr a4, int size4, IntPtr a5, int size5, IntPtr a6, int size6,
            IntPtr a7, int size7, IntPtr a8, int size8)
        {
            EventSource.EventData* descrs = stackalloc EventSource.EventData[8];
            descrs[0].DataPointer = a1;
            descrs[0].Size = size1;
            descrs[1].DataPointer = a2;
            descrs[1].Size = size2;
            descrs[2].DataPointer = a3;
            descrs[2].Size = size3;
            descrs[3].DataPointer = a4;
            descrs[3].Size = size4;
            descrs[4].DataPointer = a5;
            descrs[4].Size = size5;
            descrs[5].DataPointer = a6;
            descrs[5].Size = size6;
            descrs[6].DataPointer = a7;
            descrs[6].Size = size7;
            descrs[7].DataPointer = a8;
            descrs[7].Size = size8;
            WriteEventCore(eventId, 8, descrs);
        }
    }
}
