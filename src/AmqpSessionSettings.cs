// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;

    public sealed class AmqpSessionSettings : Begin
    {
        public AmqpSessionSettings()
        {
            this.NextOutgoingId = AmqpConstants.DefaultNextTransferId;
            this.IncomingWindow = AmqpConstants.DefaultWindowSize;
            this.OutgoingWindow = AmqpConstants.DefaultWindowSize;
            this.HandleMax = AmqpConstants.DefaultMaxLinkHandles - 1;
            this.DispositionThreshold = Math.Min(500, (int)AmqpConstants.DefaultWindowSize * 2 / 3);
            this.DispositionInterval = TimeSpan.FromMilliseconds(AmqpConstants.DefaultDispositionTimeout);
        }

        public int DispositionThreshold
        {
            get;
            set;
        }

        public TimeSpan DispositionInterval
        {
            get;
            set;
        }

        public SequenceNumber InitialDeliveryId
        {
            get;
            set;
        }

        public bool IgnoreMissingLinks
        {
            get;
            set;
        }

        public static AmqpSessionSettings Create(Begin begin)
        {
            AmqpSessionSettings settings = new AmqpSessionSettings();
            settings.Properties = begin.Properties;

            return settings;
        }

        public AmqpSessionSettings Clone()
        {
            AmqpSessionSettings settings = new AmqpSessionSettings();
            settings.DispositionThreshold = this.DispositionThreshold;
            settings.DispositionInterval = this.DispositionInterval;
            settings.NextOutgoingId = this.NextOutgoingId;
            settings.IncomingWindow = this.IncomingWindow;
            settings.OutgoingWindow = this.OutgoingWindow;
            settings.HandleMax = this.HandleMax;
            settings.OfferedCapabilities = this.OfferedCapabilities;
            settings.DesiredCapabilities = this.DesiredCapabilities;
            settings.Properties = this.Properties;

            return settings;
        }
    }
}
