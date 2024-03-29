﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the AMQP address.
    /// </summary>
    public abstract class Address
    {
        internal abstract int EncodeSize { get; }

        /// <summary>
        /// Creates an Address from a string object.
        /// </summary>
        /// <param name="value">A string representing the address.</param>
        public static implicit operator Address(string value)
        {
            return new AddressString(value);
        }

        internal static int GetEncodeSize(Address address)
        {
            return address == null ? FixedWidth.NullEncoded : address.EncodeSize;
        }

        internal static void Encode(ByteBuffer buffer, Address address)
        {
            if (address == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                address.OnEncode(buffer);
            }
        }

        internal static Address Decode(ByteBuffer buffer)
        {
            object value = AmqpEncoding.DecodeObject(buffer);
            if (value == null)
            {
                return null;
            }

            if (value is string)
            {
                return (string)value;
            }

            throw new NotSupportedException(value.GetType().ToString());
        }

        internal abstract void OnEncode(ByteBuffer buffer);

        sealed class AddressString : Address
        {
            string address;

            public AddressString(string id)
            {
                this.address = id;
            }

            internal override int EncodeSize
            {
                get { return AmqpCodec.GetStringEncodeSize(this.address); }
            }

            internal override void OnEncode(ByteBuffer buffer)
            {
                AmqpCodec.EncodeString(this.address, buffer);
            }

            public override string ToString()
            {
                return this.address;
            }

            public override int GetHashCode()
            {
                return this.address.GetHashCode();
            }

            public override bool Equals(object obj)
            {
                return obj is AddressString other &&
                    string.Equals(this.address, other.address, StringComparison.Ordinal);
            }
        }
    }
}
