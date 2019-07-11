// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Globalization;
    using System.Threading;

    /// <summary>
    /// Implements the 32-bit sequence number defined in RFC1982.
    /// </summary>
    public struct SequenceNumber : IComparable<SequenceNumber>, IEquatable<SequenceNumber>
    {
        int sequenceNumber;

        /// <summary>
        /// Initializes the struct.
        /// </summary>
        /// <param name="value">The sequence number value.</param>
        public SequenceNumber(uint value)
        {
            this.sequenceNumber = (int)value;
        }

        /// <summary>
        /// Gets the sequence number value.
        /// </summary>
        public uint Value
        {
            get { return (uint)this.sequenceNumber; }
        }

        /// <summary>
        /// Gets the next sequence number.
        /// </summary>
        /// <param name="sn">The current value.</param>
        /// <returns>The next sequence number.</returns>
        public static SequenceNumber Increment(ref int sn)
        {
            return (uint)Interlocked.Increment(ref sn);
        }

        /// <summary>
        /// Compares two integers as sequence numbers.
        /// </summary>
        /// <param name="x">The first number.</param>
        /// <param name="y">The second number.</param>
        /// <returns>A negative integer, 0, or a possitive number if the first is
        /// less than, equal or greater than the second respectively.</returns>
        public static int Compare(int x, int y)
        {
            int delta = x - y;
            if (delta == int.MinValue)
            {
                // Behavior of comparing 0u-2147483648u, 1u-2147483649u, ...
                // is undefined, so we do not allow it.
                throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpInvalidSequenceNumberComparison, x, y));
            }

            return delta;
        }

        /// <summary>
        /// Compares the current sequence number to another one.
        /// </summary>
        /// <param name="value">The other sequence number.</param>
        /// <returns>A negative integer, 0, or a possitive number if the current is
        /// less than, equal or greater than the given value respectively.</returns>
        public int CompareTo(SequenceNumber value)
        {
            return SequenceNumber.Compare(this.sequenceNumber, value.sequenceNumber);
        }

        /// <summary>
        /// Determines if the current sequence number is equal to another one.
        /// </summary>
        /// <param name="obj">The other sequence number.</param>
        /// <returns>true if they are equal, false otherwise.</returns>
        public bool Equals(SequenceNumber obj)
        {
            return this.sequenceNumber == obj.sequenceNumber;
        }

        /// <summary>
        /// Converts an unsigned interger to a sequence number.
        /// </summary>
        /// <param name="value">The unsigned integer.</param>
        public static implicit operator SequenceNumber(uint value)
        {
            return new SequenceNumber(value);
        }

        /// <summary>
        /// Adds a number to a sequence number.
        /// </summary>
        /// <param name="value1">The sequence number.</param>
        /// <param name="delta">The number to add.</param>
        /// <returns>A sequence number for the result.</returns>
        public static SequenceNumber operator +(SequenceNumber value1, int delta)
        {
            return (uint)unchecked(value1.sequenceNumber + delta);
        }

        /// <summary>
        /// Subtracts a number from a sequence number.
        /// </summary>
        /// <param name="value1">The sequence number.</param>
        /// <param name="delta">The number to subtract.</param>
        /// <returns>A sequence number for the result.</returns>
        public static SequenceNumber operator -(SequenceNumber value1, int delta)
        {
            return (uint)unchecked(value1.sequenceNumber - delta);
        }

        /// <summary>
        /// Subtracts two sequence numbers.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>The difference between the two sequence numbers.</returns>
        public static int operator -(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.sequenceNumber - value2.sequenceNumber;
        }

        /// <summary>
        /// The equal to operator.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>true if they are equal, false otherwise.</returns>
        public static bool operator ==(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.sequenceNumber == value2.sequenceNumber;
        }

        /// <summary>
        /// The not equal to operator.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>true if they are not equal, false otherwise.</returns>
        public static bool operator !=(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.sequenceNumber != value2.sequenceNumber;
        }

        /// <summary>
        /// The greater than operator.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>true if the first is greater, false otherwise.</returns>
        public static bool operator >(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) > 0;
        }

        /// <summary>
        /// The greater than or equal to operator.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>true if the first is not less, false otherwise.</returns>
        public static bool operator >=(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) >= 0;
        }

        /// <summary>
        /// The less than operator.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>true if the first is less, false otherwise.</returns>
        public static bool operator <(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) < 0;
        }

        /// <summary>
        /// The less than or equal to operator.
        /// </summary>
        /// <param name="value1">The first sequence number.</param>
        /// <param name="value2">The second sequence number.</param>
        /// <returns>true if the first is not greater, false otherwise.</returns>
        public static bool operator <=(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) <= 0;
        }

        /// <summary>
        /// Increments the sequence number.
        /// </summary>
        /// <returns>An unsigned integer.</returns>
        public uint Increment()
        {
            return (uint)unchecked(++this.sequenceNumber);
        }

        /// <summary>
        /// Returns a hash code of the sequence number.
        /// </summary>
        /// <returns>The hash code.</returns>
        public override int GetHashCode()
        {
            return this.sequenceNumber.GetHashCode();
        }

        /// <summary>
        /// Determines whether two objects are equal.
        /// </summary>
        /// <param name="obj">The object to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
        public override bool Equals(object obj)
        {
            return obj is SequenceNumber && this.Equals((SequenceNumber)obj);
        }

        /// <summary>
        /// Returns a string that represents the symbol.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return this.sequenceNumber.ToString(CultureInfo.InvariantCulture);
        }
    }
}
