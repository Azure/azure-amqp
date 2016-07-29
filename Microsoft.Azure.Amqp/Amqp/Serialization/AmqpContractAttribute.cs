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

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct,
        AllowMultiple = false, Inherited = true)]
    public sealed class AmqpContractAttribute : Attribute
    {
        public AmqpContractAttribute()
        {
            this.Encoding = EncodingType.List;
            this.Code = -1;
        }

        public string Name
        {
            get;
            set;
        }

        public long Code
        {
            get;
            set;
        }

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
