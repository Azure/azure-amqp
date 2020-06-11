// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    /// <summary>
    /// Implements the AMQP symbol type.
    /// </summary>
    public readonly struct AmqpSymbol : IEquatable<AmqpSymbol>
    {
        /// <summary>
        /// Initializes the object from a string.
        /// </summary>
        /// <param name="value">The string.</param>
        public AmqpSymbol(string value) : this()
        {
            this.Value = value;
        }

        /// <summary>
        /// Gets the string value of the symbol.
        /// </summary>
        public string Value
        {
            get;
        }

        /// <summary>
        /// Creates a symbol from a string.
        /// </summary>
        /// <param name="value">The string.</param>
        public static implicit operator AmqpSymbol(string value)
        {
            return new AmqpSymbol(value);
        }

        /// <summary>
        /// Determines whether two symbols are equal.
        /// </summary>
        /// <param name="other">The symbom to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
        public bool Equals(AmqpSymbol other)
        {
            return string.CompareOrdinal(this.Value, other.Value) == 0;
        }

        /// <summary>
        /// Returns a hash code of the symbol.
        /// </summary>
        /// <returns>The hash code.</returns>
        public override int GetHashCode()
        {
            if (this.Value == null)
            {
                return 0;
            }

            return this.Value.GetHashCode();
        }

        /// <summary>
        /// Determines whether two objects are equal.
        /// </summary>
        /// <param name="obj">The object to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
        public override bool Equals(object obj)
        {
            return (obj is AmqpSymbol) && this.Equals((AmqpSymbol)obj);
        }

        /// <summary>
        /// Returns a string that represents the symbol.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return this.Value;
        }
    }
}
