// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class Address
    {
        public abstract int EncodeSize { get; }

        public static implicit operator Address(string value)
        {
            return new AddressString(value);
        }

        public static int GetEncodeSize(Address address)
        {
            return address == null ? FixedWidth.NullEncoded : address.EncodeSize;
        }

        public static void Encode(ByteBuffer buffer, Address address)
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

        public static Address Decode(ByteBuffer buffer)
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

        public abstract void OnEncode(ByteBuffer buffer);

        sealed class AddressString : Address
        {
            string address;

            public AddressString(string id)
            {
                this.address = id;
            }

            public override int EncodeSize
            {
                get { return AmqpCodec.GetStringEncodeSize(this.address); }
            }

            public override void OnEncode(ByteBuffer buffer)
            {
                AmqpCodec.EncodeString(this.address, buffer);
            }

            public override string ToString()
            {
                return this.address;
            }
        }
    }
}
