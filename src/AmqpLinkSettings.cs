// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Settings of a link.
    /// </summary>
    public sealed class AmqpLinkSettings : Attach
    {
        uint linkCredit;

        /// <summary>
        /// Initializes the settings object.
        /// </summary>
        public AmqpLinkSettings()
        {
        }

        /// <summary>
        /// Gets or sets the total credit.
        /// </summary>
        public uint TotalLinkCredit
        {
            get
            {
                return this.linkCredit;
            }

            set
            {
                this.linkCredit = value;
                this.FlowThreshold = Math.Min(100, (int)(this.linkCredit * 2 / 3));
            }
        }

        /// <summary>
        /// The number of messages that have been settled by the application by
        /// calling one of the state update methods on <see cref="ReceivingAmqpLink"/>.
        /// A flow is sent when the threshold is reached.
        /// </summary>
        public int FlowThreshold
        {
            get;
            set;
        }

        /// <summary>
        /// Sends a flow based on <see cref="FlowThreshold"/>.
        /// </summary>
        public bool AutoSendFlow
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the <see cref="SettleMode"/> of the link.
        /// </summary>
        public SettleMode SettleType
        {
            get 
            {
                return this.SettleType(); 
            }

            set
            {
                this.SndSettleMode = null;
                this.RcvSettleMode = null;
                switch (value)
                {
                    case SettleMode.SettleOnSend:
                        this.SndSettleMode = (byte)SenderSettleMode.Settled;
                        break;
                    case SettleMode.SettleOnReceive:
                        break;
                    case SettleMode.SettleOnDispose:
                        this.RcvSettleMode = (byte)ReceiverSettleMode.Second;
                        break;
                }
            }
        }

        /// <summary>
        /// Creates a settings object from an attach.
        /// </summary>
        /// <param name="attach">The attach.</param>
        /// <returns>A AmqpLinkSettings object.</returns>
        public static AmqpLinkSettings Create(Attach attach)
        {
            AmqpLinkSettings settings = new AmqpLinkSettings();
            settings.LinkName = attach.LinkName;
            settings.Role = !attach.Role.Value;
            settings.Source = attach.Source;
            settings.Target = attach.Target;
            settings.SndSettleMode = attach.SndSettleMode;
            settings.RcvSettleMode = attach.RcvSettleMode;
            settings.MaxMessageSize = attach.MaxMessageSize;
            settings.DesiredCapabilities = attach.DesiredCapabilities;
            settings.OfferedCapabilities = attach.OfferedCapabilities;
            settings.Properties = attach.Properties;
            if (settings.Role.Value)
            {
                settings.TotalLinkCredit = AmqpConstants.DefaultLinkCredit;
                settings.AutoSendFlow = true;
            }
            else
            {
                settings.InitialDeliveryCount = 0;
            }

            return settings;
        }

        /// <summary>
        /// Determines whether two link settings are equal based on <see cref="Attach.LinkName"/>
        /// and <see cref="Attach.Role"/>. Name comparison is case insensitive.
        /// </summary>
        /// <param name="obj">The object to compare with the current object.</param>
        /// <returns>True if the specified object is equal to the current object; otherwise, false.</returns>
        public override bool Equals(object obj)
        {
            AmqpLinkSettings other = obj as AmqpLinkSettings;
            if (other == null || other.LinkName == null)
            {
                return false;
            }

            return this.LinkName.Equals(other.LinkName, StringComparison.CurrentCultureIgnoreCase) &&
                this.Role == other.Role;
        }

        /// <summary>
        /// Gets a hash code of the object.
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (this.LinkName.GetHashCode() * 397) + this.Role.GetHashCode();
        }
    }
}
