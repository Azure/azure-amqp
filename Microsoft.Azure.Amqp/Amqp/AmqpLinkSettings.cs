// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Framing;

    public sealed class AmqpLinkSettings : Attach
    {
        uint linkCredit;

        public AmqpLinkSettings()
        {
        }

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

        public long? TotalCacheSizeInBytes
        {
            get;
            set;
        }

        public int FlowThreshold
        {
            get;
            set;
        }

        public bool AutoSendFlow
        {
            get;
            set;
        }

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

        public override int GetHashCode()
        {
            return (this.LinkName.GetHashCode() * 397) + this.Role.GetHashCode();
        }
    }
}
