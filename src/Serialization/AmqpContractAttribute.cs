// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;

    /// <summary>Enumerates the encoding type associated with the serialization.</summary>
    public enum EncodingType
    {
        /// <summary>The List encoding type.</summary>
        List,
        /// <summary>The Map encoding type.</summary>
        Map
    }

    /// <summary>
    /// An attribute that applies to a class or a struct so that it can be serialized.
    /// by <see cref="AmqpContractSerializer"/>.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct,
        AllowMultiple = false, Inherited = true)]
    public sealed class AmqpContractAttribute : Attribute
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        public AmqpContractAttribute()
        {
            this.Encoding = EncodingType.List;
            this.Code = -1;
        }

        /// <summary>
        /// Gets or sets the name which is used as the descriptor.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the code which is used as the descriptor.
        /// </summary>
        public long Code
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the encoding type.
        /// </summary>
        public EncodingType Encoding
        {
            get;
            set;
        }

        internal ulong? InternalCode
        {
            get { return this.Code >= 0 ? (ulong?)this.Code : null; }
        }
    }
}
