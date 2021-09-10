// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Connection settings.
    /// </summary>
    public sealed class AmqpConnectionSettings : Open
    {
        /// <summary>
        /// Initializes the connection settings object.
        /// </summary>
        public AmqpConnectionSettings()
        {
            this.SendBufferSize = AmqpConstants.TransportBufferSize;
            this.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
            this.MaxFrameSize = AmqpConstants.DefaultMaxFrameSize;
            this.ChannelMax = AmqpConstants.DefaultMaxConcurrentChannels - 1;
            this.WriteBufferFullLimit = int.MaxValue;
            this.MinIdleTimeout = AmqpConstants.MinimumHeartBeatIntervalMs;
        }

        /// <summary>
        /// The container-id of the remote peer. This property is set only
        /// when an open performative is received from the peer.
        /// </summary>
        public string RemoteContainerId
        {
            get;
            set;
        }

        /// <summary>
        /// The hostname of the remote peer. This property is set only
        /// when an open performative is received from the peer.
        /// </summary>
        public string RemoteHostName
        {
            get;
            set;
        }

        /// <summary>
        /// Size in bytes of the send buffer used by the transport.
        /// </summary>
        public int SendBufferSize
        {
            get;
            set;
        }

        /// <summary>
        /// Size in bytes of the receive buffer used by the transport.
        /// If the value is 0, the transport adjusts the buffer according
        /// to I/O operations.
        /// </summary>
        public int ReceiveBufferSize
        {
            get;
            set;
        }

        /// <summary>
        /// A boolean value that controls how connection handles a session
        /// begin command when its channel is unknown.
        /// </summary>
        public bool IgnoreMissingSessions
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a high limit of the outgoing buffers in the async writer.
        /// When the size is above the limit, the writer raises an <see cref="IoEvent.WriteBufferQueueFull"/> event.
        /// </summary>
        public int WriteBufferFullLimit
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a low limit of the outgoing buffers in the async writer.
        /// When the size is below the limit, the writer raises an <see cref="IoEvent.WriteBufferQueueEmpty"/> event.
        /// </summary>
        public int WriteBufferEmptyLimit
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the minimum allowed connection idle-timeout value.
        /// </summary>
        public uint MinIdleTimeout
        {
            get;
            set;
        }

        /// <summary>
        /// Clones the settings object. Properties of remote peer are not copied.
        /// </summary>
        /// <returns>A new connection settings object.</returns>
        public AmqpConnectionSettings Clone()
        {
            AmqpConnectionSettings newSettings = new AmqpConnectionSettings();

            newSettings.ContainerId = this.ContainerId;
            newSettings.HostName = this.HostName;
            newSettings.MaxFrameSize = this.MaxFrameSize;
            newSettings.ChannelMax = this.ChannelMax;
            newSettings.CopyIdleTimeOut(this);
            newSettings.OutgoingLocales = this.OutgoingLocales;
            newSettings.IncomingLocales = this.IncomingLocales;
            newSettings.OfferedCapabilities = this.OfferedCapabilities;
            newSettings.DesiredCapabilities = this.DesiredCapabilities;
            newSettings.Properties = this.Properties;
            newSettings.SendBufferSize = this.SendBufferSize;
            newSettings.ReceiveBufferSize = this.ReceiveBufferSize;
            newSettings.IgnoreMissingSessions = this.IgnoreMissingSessions;
            newSettings.WriteBufferFullLimit = this.WriteBufferFullLimit;
            newSettings.WriteBufferEmptyLimit = this.WriteBufferEmptyLimit;
            newSettings.MinIdleTimeout = this.MinIdleTimeout;

            return newSettings;
        }
    }
}
