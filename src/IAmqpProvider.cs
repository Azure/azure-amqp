// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// A factory to create AMQP connections.
    /// </summary>
    public interface IConnectionFactory
    {
        /// <summary>
        /// Creates an AMQP connection.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header to be sent or being received.</param>
        /// <param name="isInitiator">true if the caller is the transport initiator, false otherwise.</param>
        /// <param name="amqpSettings">The protocol settings.</param>
        /// <param name="connectionSettings">The connection settings.</param>
        /// <returns>An AMQP connection.</returns>
        AmqpConnection CreateConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings);
    }

    /// <summary>
    /// A factory to create AMQP sessions.
    /// </summary>
    public interface ISessionFactory
    {
        /// <summary>
        /// Creates an AMQP session.
        /// </summary>
        /// <param name="connection">The connection in which the session is created.</param>
        /// <param name="settings">The session settings.</param>
        /// <returns>An AMQP session.</returns>
        AmqpSession CreateSession(AmqpConnection connection, AmqpSessionSettings settings);
    }

    /// <summary>
    /// A factory to create AMQP links.
    /// </summary>
    public interface ILinkFactory
    {
        /// <summary>
        /// Creates an AMQP link.
        /// </summary>
        /// <param name="session">The session in which the link is created.</param>
        /// <param name="settings">The link settings.</param>
        /// <returns>An AMQP link.</returns>
        AmqpLink CreateLink(AmqpSession session, AmqpLinkSettings settings);

        /// <summary>
        /// Starts an asynchronous operation to open a link.
        /// </summary>
        /// <param name="link">The link to be opened.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The object associated with this operation.</param>
        /// <returns></returns>
        IAsyncResult BeginOpenLink(AmqpLink link, TimeSpan timeout, AsyncCallback callback, object state);

        /// <summary>
        /// Ends an asynchronous operation.
        /// </summary>
        /// <param name="result">The <see cref="IAsyncResult"/> object.</param>
        void EndOpenLink(IAsyncResult result);
    }

    /// <summary>
    /// A factory for connection, session and link.
    /// </summary>
    public interface IRuntimeProvider : IConnectionFactory, ISessionFactory, ILinkFactory
    {
    }

    /// <summary>
    /// A special <see cref="IRuntimeProvider"/> that is used to support link recovery, where <see cref="AmqpLinkTerminus"/> and unsettled link deliveries are managed and potentially stored.
    /// </summary>
    public interface ILinkRecoveryRuntimeProvider : IRuntimeProvider
    {
        /// <summary>
        /// This object is used to manage <see cref="AmqpLinkTerminus"/> objects and unsettled deliveries associated with the terminus in order to support link recovery.
        /// </summary>
        IAmqpTerminusStore TerminusStore { get; }
    }

    /// <summary>
    /// Monitors the I/O operations of a transport.
    /// </summary>
    public interface ITransportMonitor
    {
        /// <summary>
        /// Called when the transport writes data.
        /// </summary>
        /// <param name="bufferSize">Size in bytes of the write buffer.</param>
        /// <param name="writeSize">Bytes of the outgoing data.</param>
        /// <param name="queueSize">Size of all currently queued buffers.</param>
        /// <param name="latencyTicks">Latency in ticks for the write operation.</param>
        void OnTransportWrite(int bufferSize, int writeSize, long queueSize, long latencyTicks);

        /// <summary>
        /// Called when the transport reads data.
        /// </summary>
        /// <param name="bufferSize">Size in bytes of the read buffer.</param>
        /// <param name="readSize">Bytes of the incoming data.</param>
        /// <param name="cacheHits">Number of read operations that were completed with data from the read buffer.</param>
        /// <param name="latencyTicks">Latency in ticks for the read operation.</param>
        void OnTransportRead(int bufferSize, int readSize, int cacheHits, long latencyTicks);
    }

    /// <summary>
    /// Records incoming and outgoing AMQP frame information.
    /// </summary>
    public interface IAmqpUsageMeter : ITransportMonitor
    {
        /// <summary>
        /// Called when a frame is read from the transport.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="frameCode">The descriptor code of the performative in the frame.</param>
        /// <param name="numberOfBytes">Bytes of the frame buffer.</param>
        void OnRead(AmqpConnection connection, ulong frameCode, int numberOfBytes);

        /// <summary>
        /// Called when a frame is written to the transport.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="frameCode">The descriptor code of the performative in the frame.</param>
        /// <param name="numberOfBytes">Bytes of the frame buffer.</param>
        void OnWrite(AmqpConnection connection, ulong frameCode, int numberOfBytes);
    }
}
