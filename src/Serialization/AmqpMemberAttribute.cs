// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;

    /// <summary>
    /// An attribute that applies to a property or a field so that it can be serialized.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field,
        AllowMultiple = false, Inherited = true)]
    public sealed class AmqpMemberAttribute : Attribute
    {
        int? order;
        bool? mandatory;

        /// <summary>
        /// Gets or sets the name. It is used as the key in <see cref="EncodingType.Map"/>.
        /// </summary>
        public string Name 
        {
            get; 
            set; 
        }

        /// <summary>
        /// Gets or sets the order. It determines the member orders in <see cref="EncodingType.List"/>.
        /// </summary>
        public int Order 
        {
            get { return this.order.HasValue ? this.order.Value : 0; }

            set { this.order = value; }
        }

        /// <summary>
        /// Gets or sets if the member is required.
        /// </summary>
        public bool Mandatory
        {
            get { return this.mandatory.HasValue ? this.mandatory.Value : false; }

            set { this.mandatory = value; }
        }

        internal int? InternalOrder
        {
            get { return this.order; }
        }

        internal bool? InternalMandatory
        {
            get { return this.mandatory; }
        }
    }
}
