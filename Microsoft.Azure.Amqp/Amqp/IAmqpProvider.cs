// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    public interface IConnectionFactory
    {
        AmqpConnection CreateConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings);
    }

    public interface ISessionFactory
    {
        AmqpSession CreateSession(AmqpConnection connection, AmqpSessionSettings settings);
    }

    public interface ILinkFactory
    {
        // It should not throw. The link object is needed to assign local/remote handles
        // so that errors in open can be sent back to peer. If exceptions must be thrown,
        // it must be done before creating the link object.
        AmqpLink CreateLink(AmqpSession session, AmqpLinkSettings settings);

        IAsyncResult BeginOpenLink(AmqpLink link, TimeSpan timeout, AsyncCallback callback, object state);

        void EndOpenLink(IAsyncResult result);
    }

    public interface IRuntimeProvider : IConnectionFactory, ISessionFactory, ILinkFactory
    {
    }

    public interface INodeFactory
    {
        IAsyncResult BeginCreateNode(string address, Fields properties, TimeSpan timeout, AsyncCallback callback, object state);

        void EndCreateNode(IAsyncResult result);

        IAsyncResult BeginDeleteNode(string address, TimeSpan timeout, AsyncCallback callback, object state);

        void EndDeleteNode(IAsyncResult result);
    }

    public interface IAmqpProvider : IRuntimeProvider, INodeFactory
    {
    }

    public interface IAmqpUsageMeter
    {
        void OnRead(AmqpConnection connection, ulong frameCode, int numberOfBytes);

        void OnWrite(AmqpConnection connection, ulong frameCode, int numberOfBytes);
    }
}
