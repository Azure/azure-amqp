// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class Terminus
    {
        Source source;
        Target target;

        public Terminus(Source source)
        {
            Fx.Assert(source != null, "source cannot be null");
            this.source = source;
        }

        public Terminus(Target target)
        {
            Fx.Assert(target != null, "target cannot be null");
            this.target = target;
        }

        public Address Address
        {
            get
            {
                return this.source != null ? this.source.Address : this.target.Address;
            }
        }

        public TerminusDurability Durable
        {
            get
            {
                if (this.source != null)
                {
                    return this.source.Durable == null ? TerminusDurability.None : (TerminusDurability)this.source.Durable.Value;
                }
                else
                {
                    return this.target.Durable == null ? TerminusDurability.None : (TerminusDurability)this.target.Durable.Value;
                }
            }
        }

        public AmqpSymbol ExpiryPolicy
        {
            get
            {
                return this.source != null ? this.source.ExpiryPolicy : this.target.ExpiryPolicy;
            }
        }

        public uint? Timeout
        {
            get
            {
                return this.source != null ? this.source.Timeout : this.target.Timeout;
            }
        }

        public bool? Dynamic
        {
            get
            {
                return this.source != null ? this.source.Dynamic : this.target.Dynamic;
            }
        }

        public AmqpMap DynamicNodeProperties
        {
            get
            {
                return this.source != null ? this.source.DynamicNodeProperties : this.target.DynamicNodeProperties;
            }
        }

        public Multiple<AmqpSymbol> Capabilities
        {
            get
            {
                return this.source != null ? this.source.Capabilities : this.target.Capabilities;
            }
        }
    }
}
