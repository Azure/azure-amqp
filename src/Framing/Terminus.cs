// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Provides a unified view of source and target terminus properties.
    /// </summary>
    [Obsolete("Use Source and Target directly.")]
    public sealed class Terminus
    {
        Source source;
        Target target;

        /// <summary>Initializes a new instance from a source.</summary>
        public Terminus(Source source)
        {
            Fx.Assert(source != null, "source cannot be null");
            this.source = source;
        }

        /// <summary>Initializes a new instance from a target.</summary>
        public Terminus(Target target)
        {
            Fx.Assert(target != null, "target cannot be null");
            this.target = target;
        }

        /// <summary>Gets the address.</summary>
        public Address Address
        {
            get
            {
                return this.source != null ? this.source.Address : this.target.Address;
            }
        }

        /// <summary>Gets the durability setting.</summary>
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

        /// <summary>Gets the expiry policy.</summary>
        public AmqpSymbol ExpiryPolicy
        {
            get
            {
                return this.source != null ? this.source.ExpiryPolicy : this.target.ExpiryPolicy;
            }
        }

        /// <summary>Gets the timeout.</summary>
        public uint? Timeout
        {
            get
            {
                return this.source != null ? this.source.Timeout : this.target.Timeout;
            }
        }

        /// <summary>Gets whether the terminus is dynamic.</summary>
        public bool? Dynamic
        {
            get
            {
                return this.source != null ? this.source.Dynamic : this.target.Dynamic;
            }
        }

        /// <summary>Gets the dynamic node properties.</summary>
        public AmqpMap DynamicNodeProperties
        {
            get
            {
                return this.source != null ? this.source.DynamicNodeProperties : this.target.DynamicNodeProperties;
            }
        }

        /// <summary>Gets the capabilities.</summary>
        public Multiple<AmqpSymbol> Capabilities
        {
            get
            {
                return this.source != null ? this.source.Capabilities : this.target.Capabilities;
            }
        }
    }
}
