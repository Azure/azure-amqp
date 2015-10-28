// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    sealed class SerialiableMember
    {
        public string Name
        {
            get;
            set;
        }

        public int Order
        {
            get;
            set;
        }

        public bool Mandatory
        {
            get;
            set;
        }

        public MemberAccessor Accessor
        {
            get;
            set;
        }

        public SerializableType Type
        {
            get;
            set;
        }
    }
}
