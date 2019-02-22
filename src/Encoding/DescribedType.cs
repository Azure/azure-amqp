// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    /// <summary>
    /// Implements the AMQP described type.
    /// </summary>
    public class DescribedType
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="descriptor">The descriptor.</param>
        /// <param name="value">The described value.</param>
        public DescribedType(object descriptor, object value)
        {
            this.Descriptor = descriptor;
            this.Value = value;
        }

        /// <summary>
        /// Gets the descriptor.
        /// </summary>
        public object Descriptor
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public object Value
        {
            get;
            set;
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return $"{this.Descriptor}:{this.Value}";
        }
    }
}
