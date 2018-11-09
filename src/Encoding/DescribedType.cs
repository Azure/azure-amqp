// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    public class DescribedType
    {
        public DescribedType(object descriptor, object value)
        {
            this.Descriptor = descriptor;
            this.Value = value;
        }

        public object Descriptor
        {
            get;
            set;
        }

        public object Value
        {
            get;
            set;
        }

        public override string ToString()
        {
            return this.Value == null ? string.Empty : this.Value.ToString();
        }
    }
}
