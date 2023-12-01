// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Globalization;

    public struct FormatCode : IEquatable<FormatCode>
    {
        public const byte Described = 0x00;

        // fixed width
        public const byte Null = 0x40;
        public const byte Boolean = 0x56;
        public const byte BooleanTrue = 0x41;
        public const byte BooleanFalse = 0x42;
        public const byte UInt0 = 0x43;
        public const byte ULong0 = 0x44;
        public const byte UByte = 0x50;
        public const byte UShort = 0x60;
        public const byte UInt = 0x70;
        public const byte ULong = 0x80;
        public const byte Byte = 0x51;
        public const byte Short = 0x61;
        public const byte Int = 0x71;
        public const byte Long = 0x81;
        public const byte SmallUInt = 0x52;
        public const byte SmallULong = 0x53;
        public const byte SmallInt = 0x54;
        public const byte SmallLong = 0x55;
        public const byte Float = 0x72;
        public const byte Double = 0x82;
        public const byte Decimal32 = 0x74;
        public const byte Decimal64 = 0x84;
        public const byte Decimal128 = 0x94;
        public const byte Char = 0x73;
        public const byte TimeStamp = 0x83;
        public const byte Uuid = 0x98;

        // variable
        public const byte Binary8 = 0xa0;
        public const byte Binary32 = 0xb0;
        public const byte String8Utf8 = 0xa1;
        public const byte String32Utf8 = 0xb1;
        public const byte Symbol8 = 0xa3;
        public const byte Symbol32 = 0xb3;

        // compound
        public const byte List0 = 0x45;
        public const byte List8 = 0xc0;
        public const byte List32 = 0xd0;
        public const byte Map8 = 0xc1;
        public const byte Map32 = 0xd1;
        public const byte Array8 = 0xe0;
        public const byte Array32 = 0xf0;

        byte type;
        byte extType;

        public FormatCode(byte type) :
            this(type, 0)
        {
        }

        public FormatCode(byte type, byte extType)
        {
            this.type = type;
            this.extType = extType;
        }

        public byte Type
        {
            get { return this.type; }
        }

        public byte SubType
        {
            get { return (byte)(this.type & 0x0F); }
        }

        public byte SubCategory
        {
            get { return (byte)((this.type & 0xF0) >> 4); }
        }

        public byte ExtType
        {
            get { return this.extType; }
        }

        public static bool HasExtType(byte type)
        {
            return (type & 0xF) == 0xF;
        }
 
        public static implicit operator FormatCode(byte value)
        {
            return new FormatCode(value);
        }

        public static implicit operator byte(FormatCode value)
        {
            return value.Type;
        }

        public static bool operator ==(FormatCode fc1, FormatCode fc2)
        {
            return fc1.Type == fc2.Type;
        }

        public static bool operator !=(FormatCode fc1, FormatCode fc2)
        {
            return fc1.Type != fc2.Type;
        }

        public bool HasExtType()
        {
            return (this.type & 0xF) == 0xF;
        }

        public override bool Equals(object obj)
        {            
            return obj is FormatCode && this == (FormatCode)obj;
        }

        public override int GetHashCode()
        {
            return this.type.GetHashCode();
        }

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

        bool IEquatable<FormatCode>.Equals(FormatCode other)
        {
            return this == other;
        }
    }
}
