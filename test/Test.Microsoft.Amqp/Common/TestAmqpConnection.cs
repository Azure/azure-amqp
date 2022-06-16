// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Transport;
    using System.Collections.Generic;

    public class TestAmqpConnection : AmqpConnection
    {
        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        public TestAmqpConnection(TransportBase transport, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this(transport, amqpSettings.GetDefaultHeader(), true, amqpSettings, connectionSettings)
        {
        }

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header for version negotiation.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        public TestAmqpConnection(TransportBase transport, ProtocolHeader protocolHeader, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this(transport, protocolHeader, true, amqpSettings, connectionSettings)
        {
        }

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header for version negotiation.</param>
        /// <param name="isInitiator">True if the connection is the initiator; false otherwise.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        public TestAmqpConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) :
            this((isInitiator ? "out" : "in") + "-connection", transport, protocolHeader, isInitiator, amqpSettings, connectionSettings)
        {
        }

        /// <summary>
        /// Initializes a connection object.
        /// </summary>
        /// <param name="type">A prefix for the connection name for debugging purposes.</param>
        /// <param name="transport">The transport for the connection.</param>
        /// <param name="protocolHeader">The protocol header for version negotiation.</param>
        /// <param name="isInitiator">True if the connection is the initiator; false otherwise.</param>
        /// <param name="amqpSettings">The protocol settings <see cref="AmqpSettings"/>.</param>
        /// <param name="connectionSettings">The connection settings <see cref="AmqpConnectionSettings"/>.</param>
        protected TestAmqpConnection(string type, TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings) 
            : base(type, transport, protocolHeader, isInitiator, amqpSettings, connectionSettings)
        {
            this.ReceivedPerformatives = new LinkedList<Performative>();
        }

        internal LinkedList<Performative> ReceivedPerformatives
        {
            get;
        }

        /// <summary>
        /// Handles the received frame buffer.
        /// </summary>
        /// <param name="buffer">The received frame buffer.</param>
        protected override void OnFrameBuffer(ByteBuffer buffer)
        {
            if (this.State == AmqpObjectState.End)
            {
                buffer.Dispose();
                return;
            }

            using (Frame frame = new Frame())
            {
                frame.Decode(buffer);
                if (frame.Command != null)
                {
                    this.ReceivedPerformatives.AddLast(frame.Command);
                    this.ProcessFrame(frame);
                }
            }
        }
    }
}
