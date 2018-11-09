// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using Microsoft.Azure.Amqp.Framing;

    public sealed class AmqpConnectionSettings : Open
    {
        public AmqpConnectionSettings()
        {
            this.SendBufferSize = AmqpConstants.TransportBufferSize;
            this.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
            this.MaxFrameSize = AmqpConstants.DefaultMaxFrameSize;
            this.ChannelMax = AmqpConstants.DefaultMaxConcurrentChannels - 1;
            this.WriteBufferFullLimit = int.MaxValue;
            this.MinIdleTimeout = AmqpConstants.MinimumHeartBeatIntervalMs;
        }

        public string RemoteContainerId
        {
            get;
            set;
        }

        public string RemoteHostName
        {
            get;
            set;
        }

        public int SendBufferSize
        {
            get;
            set;
        }

        public int ReceiveBufferSize
        {
            get;
            set;
        }

        public bool IgnoreMissingSessions
        {
            get;
            set;
        }

        public int WriteBufferFullLimit
        {
            get;
            set;
        }

        public int WriteBufferEmptyLimit
        {
            get;
            set;
        }

        public uint MinIdleTimeout
        {
            get;
            set;
        }

        /// <summary> doesn't clone - RemoteSettings: HostName/ContainerId </summary>
        public AmqpConnectionSettings Clone()
        {
            AmqpConnectionSettings newSettings = new AmqpConnectionSettings();

            newSettings.ContainerId = this.ContainerId;
            newSettings.HostName = this.HostName;
            newSettings.MaxFrameSize = this.MaxFrameSize;
            newSettings.ChannelMax = this.ChannelMax;
            newSettings.IdleTimeOut = this.IdleTimeOut;
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
