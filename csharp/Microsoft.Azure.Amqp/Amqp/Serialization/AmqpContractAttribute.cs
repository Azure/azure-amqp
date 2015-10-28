// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;

    public enum EncodingType
    {
        List,
        Map
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct,
        AllowMultiple = false, Inherited = true)]
    sealed class AmqpContractAttribute : Attribute
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
