// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Globalization;

    /// <summary>
    /// Represents an AMQP type format code.
    /// </summary>
    public readonly struct FormatCode : IEquatable<FormatCode>
    {
        /// <summary>Format code for described type.</summary>
        public const byte Described = 0x00;

        // fixed width
        /// <summary>Format code for null.</summary>
        public const byte Null = 0x40;
        /// <summary>Format code for boolean.</summary>
        public const byte Boolean = 0x56;
        /// <summary>Format code for boolean true.</summary>
        public const byte BooleanTrue = 0x41;
        /// <summary>Format code for boolean false.</summary>
        public const byte BooleanFalse = 0x42;
        /// <summary>Format code for uint zero.</summary>
        public const byte UInt0 = 0x43;
        /// <summary>Format code for ulong zero.</summary>
        public const byte ULong0 = 0x44;
        /// <summary>Format code for unsigned byte.</summary>
        public const byte UByte = 0x50;
        /// <summary>Format code for unsigned short.</summary>
        public const byte UShort = 0x60;
        /// <summary>Format code for unsigned int.</summary>
        public const byte UInt = 0x70;
        /// <summary>Format code for unsigned long.</summary>
        public const byte ULong = 0x80;
        /// <summary>Format code for signed byte.</summary>
        public const byte Byte = 0x51;
        /// <summary>Format code for signed short.</summary>
        public const byte Short = 0x61;
        /// <summary>Format code for signed int.</summary>
        public const byte Int = 0x71;
        /// <summary>Format code for signed long.</summary>
        public const byte Long = 0x81;
        /// <summary>Format code for small unsigned int.</summary>
        public const byte SmallUInt = 0x52;
        /// <summary>Format code for small unsigned long.</summary>
        public const byte SmallULong = 0x53;
        /// <summary>Format code for small signed int.</summary>
        public const byte SmallInt = 0x54;
        /// <summary>Format code for small signed long.</summary>
        public const byte SmallLong = 0x55;
        /// <summary>Format code for float.</summary>
        public const byte Float = 0x72;
        /// <summary>Format code for double.</summary>
        public const byte Double = 0x82;
        /// <summary>Format code for decimal32.</summary>
        public const byte Decimal32 = 0x74;
        /// <summary>Format code for decimal64.</summary>
        public const byte Decimal64 = 0x84;
        /// <summary>Format code for decimal128.</summary>
        public const byte Decimal128 = 0x94;
        /// <summary>Format code for char.</summary>
        public const byte Char = 0x73;
        /// <summary>Format code for timestamp.</summary>
        public const byte TimeStamp = 0x83;
        /// <summary>Format code for uuid.</summary>
        public const byte Uuid = 0x98;

        // variable
        /// <summary>Format code for small binary.</summary>
        public const byte Binary8 = 0xa0;
        /// <summary>Format code for large binary.</summary>
        public const byte Binary32 = 0xb0;
        /// <summary>Format code for small UTF-8 string.</summary>
        public const byte String8Utf8 = 0xa1;
        /// <summary>Format code for large UTF-8 string.</summary>
        public const byte String32Utf8 = 0xb1;
        /// <summary>Format code for small symbol.</summary>
        public const byte Symbol8 = 0xa3;
        /// <summary>Format code for large symbol.</summary>
        public const byte Symbol32 = 0xb3;

        // compound
        /// <summary>Format code for empty list.</summary>
        public const byte List0 = 0x45;
        /// <summary>Format code for small list.</summary>
        public const byte List8 = 0xc0;
        /// <summary>Format code for large list.</summary>
        public const byte List32 = 0xd0;
        /// <summary>Format code for small map.</summary>
        public const byte Map8 = 0xc1;
        /// <summary>Format code for large map.</summary>
        public const byte Map32 = 0xd1;
        /// <summary>Format code for small array.</summary>
        public const byte Array8 = 0xe0;
        /// <summary>Format code for large array.</summary>
        public const byte Array32 = 0xf0;

        readonly byte type;
        readonly byte extType;

        /// <summary>Initializes a new instance with the specified type.</summary>
        public FormatCode(byte type) :
            this(type, 0)
        {
        }

        /// <summary>Initializes a new instance with the specified type and extended type.</summary>
        public FormatCode(byte type, byte extType)
        {
            this.type = type;
            this.extType = extType;
        }

        /// <summary>Gets the type byte.</summary>
        public byte Type
        {
            get { return this.type; }
        }

        /// <summary>Gets the sub-type byte.</summary>
        public byte SubType
        {
            get { return (byte)(this.type & 0x0F); }
        }

        /// <summary>Gets the sub-category byte.</summary>
        public byte SubCategory
        {
            get { return (byte)((this.type & 0xF0) >> 4); }
        }

        /// <summary>Gets the extended type byte.</summary>
        public byte ExtType
        {
            get { return this.extType; }
        }

        /// <summary>Determines whether the specified type has an extended type.</summary>
        public static bool HasExtType(byte type)
        {
            return (type & 0xF) == 0xF;
        }

        /// <summary>Implicitly converts a byte to a FormatCode.</summary>
        public static implicit operator FormatCode(byte value)
        {
            return new FormatCode(value);
        }

        /// <summary>Implicitly converts a FormatCode to a byte.</summary>
        public static implicit operator byte(FormatCode value)
        {
            return value.Type;
        }

        /// <summary>Determines whether two FormatCode values are equal.</summary>
        public static bool operator ==(FormatCode fc1, FormatCode fc2)
        {
            return fc1.Type == fc2.Type;
        }

        /// <summary>Determines whether two FormatCode values are not equal.</summary>
        public static bool operator !=(FormatCode fc1, FormatCode fc2)
        {
            return fc1.Type != fc2.Type;
        }

        /// <summary>Determines whether this instance has an extended type.</summary>
        public bool HasExtType()
        {
            return (this.type & 0xF) == 0xF;
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is FormatCode && this == (FormatCode)obj;
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            return this.type.GetHashCode();
        }

        /// <inheritdoc/>
        public override string ToString()
        {
            if (this.HasExtType())
            {
                return string.Format(CultureInfo.InvariantCulture, "0x{0:X2}.{1:X2}", this.Type, this.ExtType);
            }
            else
            {
                return string.Format(CultureInfo.InvariantCulture, "0x{0:X2}", this.Type);
            }
        }

        /// <summary>Determines whether this instance is equal to another FormatCode.</summary>
        public bool Equals(FormatCode other)
        {
            return this.type == other.type;
        }
    }
}
