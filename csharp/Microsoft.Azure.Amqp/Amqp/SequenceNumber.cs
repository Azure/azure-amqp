// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Globalization;
    using System.Threading;

    /// <summary>
    /// RFC1982: http://tools.ietf.org/html/rfc1982
    /// </summary>
    public struct SequenceNumber : IComparable<SequenceNumber>, IEquatable<SequenceNumber>
    {
        int sequenceNumber;

        public SequenceNumber(uint value)
        {
            this.sequenceNumber = (int)value;
        }

        public uint Value
        {
            get { return (uint)this.sequenceNumber; }
        }

        public static SequenceNumber Increment(ref int sn)
        {
            return (uint)Interlocked.Increment(ref sn);
        }

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

        public int CompareTo(SequenceNumber value)
        {
            return SequenceNumber.Compare(this.sequenceNumber, value.sequenceNumber);
        }

        public bool Equals(SequenceNumber obj)
        {
            return this.sequenceNumber == obj.sequenceNumber;
        }

        public static implicit operator SequenceNumber(uint value)
        {
            return new SequenceNumber(value);
        }

        public static SequenceNumber operator +(SequenceNumber value1, int delta)
        {
            return (uint)unchecked(value1.sequenceNumber + delta);
        }

        public static SequenceNumber operator -(SequenceNumber value1, int delta)
        {
            return (uint)unchecked(value1.sequenceNumber - delta);
        }

        public static int operator -(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.sequenceNumber - value2.sequenceNumber;
        }

        public static bool operator ==(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.sequenceNumber == value2.sequenceNumber;
        }

        public static bool operator !=(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.sequenceNumber != value2.sequenceNumber;
        }

        public static bool operator >(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) > 0;
        }

        public static bool operator >=(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) >= 0;
        }

        public static bool operator <(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) < 0;
        }

        public static bool operator <=(SequenceNumber value1, SequenceNumber value2)
        {
            return value1.CompareTo(value2) <= 0;
        }

        public uint Increment()
        {
            return (uint)unchecked(++this.sequenceNumber);
        }

        public override int GetHashCode()
        {
            return this.sequenceNumber.GetHashCode();
        }

        public override bool Equals(object obj)
        {
            return obj is SequenceNumber && this.Equals((SequenceNumber)obj);
        }

        public override string ToString()
        {
            return this.sequenceNumber.ToString(CultureInfo.InvariantCulture);
        }
    }
}
