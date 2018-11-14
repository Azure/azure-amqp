// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    public struct AmqpSymbol : IEquatable<AmqpSymbol>
    {
        int valueSize;

        public AmqpSymbol(string value) : this()
        {
            this.Value = value;
        }

        public string Value
        {
            get;
            private set;
        }

        public int ValueSize
        {
            get
            {
                if (this.valueSize == 0)
                {
                    this.valueSize = SymbolEncoding.GetValueSize(this);
                }

                return this.valueSize;
            }
        }

        public static implicit operator AmqpSymbol(string value)
        {
            return new AmqpSymbol() { Value = value };
        }

        public bool Equals(AmqpSymbol other)
        {
            if (this.Value == null && other.Value == null)
            {
                return true;
            }

            if (this.Value == null || other.Value == null)
            {
                return false;
            }

            return string.Compare(this.Value, other.Value, StringComparison.Ordinal) == 0;
        }

        public override int GetHashCode()
        {
            if (this.Value == null)
            {
                return 0;
            }

            return this.Value.GetHashCode();
        }

        public override bool Equals(object obj)
        {
            return (obj is AmqpSymbol) && this.Equals((AmqpSymbol)obj);
        }

        public override string ToString()
        {
            return this.Value;
        }
    }
}
