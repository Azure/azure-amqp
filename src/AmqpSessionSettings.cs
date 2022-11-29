// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Settings for a session object.
    /// </summary>
    public sealed class AmqpSessionSettings : Begin
    {
        /// <summary>
        /// Initializes the settings.
        /// </summary>
        public AmqpSessionSettings()
        {
            this.NextOutgoingId = AmqpConstants.DefaultNextTransferId;
            this.IncomingWindow = AmqpConstants.DefaultWindowSize;
            this.OutgoingWindow = AmqpConstants.DefaultWindowSize;
            this.HandleMax = AmqpConstants.DefaultMaxLinkHandles - 1;
            this.DispositionThreshold = Math.Min(500, (int)AmqpConstants.DefaultWindowSize * 2 / 3);
        }

        /// <summary>
        /// The threshold to move session window forward after that many
        /// transfers are processed.
        /// </summary>
        public int DispositionThreshold
        {
            get;
            set;
        }

        /// <summary>
        /// A delay to send dispositions after a delivery state is updated. It allows
        /// multiple deliveries to be batched in one disposition. TimeSpan.Zero means no delay.
        /// </summary>
        public TimeSpan DispositionInterval
        {
            get;
            set;
        }

        /// <summary>
        /// The initial value of delivery ids.
        /// </summary>
        public SequenceNumber InitialDeliveryId
        {
            get;
            set;
        }

        /// <summary>
        /// Determines if the session should ignore an attach
        /// whole handle is unknown.
        /// </summary>
        public bool IgnoreMissingLinks
        {
            get;
            set;
        }

        internal static AmqpSessionSettings Create(Begin begin)
        {
            AmqpSessionSettings settings = new AmqpSessionSettings();
            settings.Properties = begin.Properties;

            return settings;
        }

        /// <summary>
        /// Clones the current settings object.
        /// </summary>
        /// <returns>A new settings object.</returns>
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
