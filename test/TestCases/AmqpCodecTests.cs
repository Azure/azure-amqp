namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.Serialization;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Sasl;
    using global::Microsoft.Azure.Amqp.Serialization;
    using global::Microsoft.Azure.Amqp.Transaction;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpCodecTests
    {
        bool boolTrue = true;
        byte[] boolTrueBin = new byte[] { 0x41 };
        byte[] boolTrueBin1 = new byte[] { 0x56, 0x01 };

        bool boolFalse = false;
        byte[] boolFalseBin = new byte[] { 0x42 };
        byte[] boolFalseBin1 = new byte[] { 0x56, 0x00 };

        byte ubyteValue = 0x33;
        byte[] ubyteValueBin = new byte[] { 0x50, 0x33 };

        ushort ushortValue = 0x1234;
        byte[] ushortValueBin = new byte[] { 0x60,  0x12, 0x34};

        uint uint0Value = 0x00;
        byte[] uint0ValueBin = new byte[] { 0x43 };

        uint uintSmallValue = 0xe1;
        byte[] uintSmallValueBin = new byte[] { 0x52, 0xe1 };

        uint uintValue = 0xedcba098;
        byte[] uintValueBin = new byte[] { 0x70, 0xed, 0xcb, 0xa0, 0x98 };

        ulong ulong0Value = 0x00;
        byte[] ulong0ValueBin = new byte[] { 0x44 };

        ulong ulongSmallValue = 0xf2;
        byte[] ulongSmallValueBin = new byte[] { 0x53, 0xf2 };

        ulong ulongValue = 0x12345678edcba098;
        byte[] ulongValueBin = new byte[] { 0x80, 0x12, 0x34, 0x56, 0x78, 0xed, 0xcb, 0xa0, 0x98 };

        sbyte byteValue = -20;
        byte[] byteValueBin = new byte[] { 0x51, 0xec };

        short shortValue = 0x5678;
        byte[] shortValueBin = new byte[] { 0x61, 0x56, 0x78 };

        int intSmallValue = -77;
        byte[] intSmallValueBin = new byte[] { 0x54, 0xb3 };

        int intValue = 0x56789a00;
        byte[] intValueBin = new byte[] { 0x71, 0x56, 0x78, 0x9a, 0x00 };

        long longSmallValue = 0x22;
        byte[] longSmallValueBin = new byte[] { 0x55, 0x22 };

        long longValue = -111111111111; //FFFFFFE62142FE39
        byte[] longValueBin = new byte[] { 0x81, 0xff, 0xff, 0xff, 0xe6, 0x21, 0x42, 0xfe, 0x39 };

        float floatValue = -88.88f;
        byte[] floatValueBin = new byte[] { 0x72, 0xc2, 0xb1, 0xc2, 0x8f };

        double doubleValue = 111111111111111.22222222222;
        byte[] doubleValueBin = new byte[] { 0x82, 0x42, 0xd9, 0x43, 0x84, 0x93, 0xbc, 0x71, 0xce };

        decimal decimal32Value = 123.4567M; // 0x12D687 * 10 (0x61 - 101)
        byte[] decimal32ValueBin = new byte[] { 0x74, 0x30, 0x92, 0xd6, 0x87 };

        decimal decimal64Value = -1234567899.999988M; // s=0x462D53D216EF4, e = 0x188
        byte[] decimal64ValueBin = new byte[] { 0x84, 0xb1, 0x04, 0x62, 0xd5, 0x3d, 0x21, 0x6e, 0xf4 };

        decimal decimal128Value = decimal.MaxValue; // s=0xffffffffffff, e = 6176
        byte[] decimal128ValueBin = new byte[] { 0x94, 0x30, 0x40, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

        char charValue = 'A';
        byte[] charValueBin = new byte[] { 0x73, 0x00, 0x00, 0x00, 0x41 };

        DateTime dtValue = DateTime.Parse("2008-11-01T19:35:00.0000000Z").ToUniversalTime();
        byte[] dtValueBin = new byte[] { 0x83, 0x00, 0x00, 0x01, 0x1d, 0x59, 0x8d, 0x1e, 0xa0 };

        Guid uuidValue = new Guid(0xf275ea5e, 0x0c57, 0x4ad7, 0xb1, 0x1a, 0xb2, 0x0c, 0x56, 0x3d, 0x3b, 0x71);
        byte[] uuidValueBin = new byte[] { 0x98, 0xf2, 0x75, 0xea, 0x5e, 0x0c, 0x57, 0x4a, 0xd7, 0xb1, 0x1a, 0xb2, 0x0c, 0x56, 0x3d, 0x3b, 0x71 };

        static byte[] binData = new byte[512];
        const int bin8Len = 56;
        const int bin32Len = 512;
        byte[] bin8ValueBin = new byte[1 + 1 + bin8Len];
        byte[] bin32ValueBin = new byte[1 + 4 + bin32Len];
        ArraySegment<byte> bin8Value = new ArraySegment<byte>(binData, 0, bin8Len);
        ArraySegment<byte> bin32Value = new ArraySegment<byte>(binData, 0, bin32Len);

        string strValue = "amqp";
        string str32Value = new string('A', 512);
        byte[] sym8ValueBin = new byte[] { 0xa3, 0x04, 0x61, 0x6d, 0x71, 0x70 };
        byte[] sym32ValueBin = new byte[] { 0xb3, 0x00, 0x00, 0x00, 0x04, 0x61, 0x6d, 0x71, 0x70 };

        byte[] str8Utf8ValueBin = new byte[] { 0xa1, 0x04, 0x61, 0x6d, 0x71, 0x70 };
        byte[] str32Utf8ValueBin = new byte[] { 0xb1, 0x00, 0x00, 0x00, 0x04, 0x61, 0x6d, 0x71, 0x70 };

        DescribedType described1 = new DescribedType((int)100, "value1");
        DescribedType described2 = new DescribedType((AmqpSymbol)"v2", (float)3.14159);
        DescribedType described3 = new DescribedType("v3", Guid.NewGuid());
        DescribedType described4 = new DescribedType(DateTime.Now, new List<object>() { 100, "200" });
        DescribedType described5 = new DescribedType(12345L, new string[] { "string1", "string2", "string3", "string4" });

        public AmqpCodecTests()
        {
            bin8ValueBin[0] = 0xa0;
            bin8ValueBin[1] = bin8Len;
            bin32ValueBin[0] = 0xb0;
            bin32ValueBin[1] = 0x00;
            bin32ValueBin[2] = 0x00;
            bin32ValueBin[3] = 0x02;
            bin32ValueBin[4] = 0x00;
        }

        [Fact]
        public void AmqpCodecSingleValueTest()
        {
            byte[] workBuffer = new byte[2048];
            ByteBuffer buffer;

            // boolean true
            AmqpCodec.EncodeBoolean(boolTrue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(boolTrueBin, 0, boolTrueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            bool? bv = AmqpCodec.DecodeBoolean(new ByteBuffer(new ArraySegment<byte>(boolTrueBin)));
            Assert.True(bv.Value, "Boolean value is not true.");

            // boolean false
            AmqpCodec.EncodeBoolean(boolFalse, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(boolFalseBin, 0, boolFalseBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            bv = AmqpCodec.DecodeBoolean(new ByteBuffer(new ArraySegment<byte>(boolFalseBin)));
            Assert.False(bv.Value, "Boolean value is not false.");

            // ubyte
            AmqpCodec.EncodeUByte(ubyteValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ubyteValueBin, 0, ubyteValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            byte? bytev = AmqpCodec.DecodeUByte(new ByteBuffer(new ArraySegment<byte>(ubyteValueBin)));
            Assert.True(bytev == ubyteValue, "UByte value is not equal.");

            // ushort
            AmqpCodec.EncodeUShort(ushortValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ushortValueBin, 0, ushortValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ushort? ushortv = AmqpCodec.DecodeUShort(new ByteBuffer(new ArraySegment<byte>(ushortValueBin)));
            Assert.True(ushortv == ushortValue, "UShort value is not equal.");

            // uint0
            AmqpCodec.EncodeUInt(uint0Value, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uint0ValueBin, 0, uint0ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            uint? uint0v = AmqpCodec.DecodeUInt(new ByteBuffer(new ArraySegment<byte>(uint0ValueBin)));
            Assert.True(uint0v == uint0Value, "UInt0 value is not equal.");

            // uint small
            AmqpCodec.EncodeUInt(uintSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uintSmallValueBin, 0, uintSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            uint? uintSmallV = AmqpCodec.DecodeUInt(new ByteBuffer(new ArraySegment<byte>(uintSmallValueBin)));
            Assert.True(uintSmallV == uintSmallValue, "UIntSmall value is not equal.");

            // uint
            AmqpCodec.EncodeUInt(uintValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uintValueBin, 0, uintValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            uint? uintv = AmqpCodec.DecodeUInt(new ByteBuffer(new ArraySegment<byte>(uintValueBin)));
            Assert.True(uintv == uintValue, "UInt value is not equal.");

            // ulong0
            AmqpCodec.EncodeULong(ulong0Value, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ulong0ValueBin, 0, ulong0ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ulong? ulong0v = AmqpCodec.DecodeULong(new ByteBuffer(new ArraySegment<byte>(ulong0ValueBin)));
            Assert.True(ulong0v == ulong0Value, "ULong0 value is not equal.");

            // ulong small
            AmqpCodec.EncodeULong(ulongSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ulongSmallValueBin, 0, ulongSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ulong? ulongSmallV = AmqpCodec.DecodeULong(new ByteBuffer(new ArraySegment<byte>(ulongSmallValueBin)));
            Assert.True(ulongSmallV == ulongSmallValue, "ULong value is not equal.");

            // ulong
            AmqpCodec.EncodeULong(ulongValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ulongValueBin, 0, ulongValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ulong? ulongv = AmqpCodec.DecodeULong(new ByteBuffer(new ArraySegment<byte>(ulongValueBin)));
            Assert.True(ulongv == ulongValue, "ULong value is not equal.");

            // byte
            AmqpCodec.EncodeByte(byteValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(byteValueBin, 0, byteValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            sbyte? sbytev = AmqpCodec.DecodeByte(new ByteBuffer(new ArraySegment<byte>(byteValueBin)));
            Assert.True(sbytev == byteValue, "Byte value is not equal.");

            // short
            AmqpCodec.EncodeShort(shortValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(shortValueBin, 0, shortValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            short? shortv = AmqpCodec.DecodeShort(new ByteBuffer(new ArraySegment<byte>(shortValueBin)));
            Assert.True(shortv == shortValue, "Short value is not equal.");

            // int small
            AmqpCodec.EncodeInt(intSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(intSmallValueBin, 0, intSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            int? intSmallV = AmqpCodec.DecodeInt(new ByteBuffer(new ArraySegment<byte>(intSmallValueBin)));
            Assert.True(intSmallV == intSmallValue, "Int small value is not equal.");

            // int
            AmqpCodec.EncodeInt(intValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(intValueBin, 0, intValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            int? intv = AmqpCodec.DecodeInt(new ByteBuffer(new ArraySegment<byte>(intValueBin)));
            Assert.True(intv == intValue, "Int value is not equal.");

            // long
            AmqpCodec.EncodeLong(longSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(longSmallValueBin, 0, longSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            long? longSmallV = AmqpCodec.DecodeLong(new ByteBuffer(new ArraySegment<byte>(longSmallValueBin)));
            Assert.True(longSmallV == longSmallValue, "Long small value is not equal.");

            // long
            AmqpCodec.EncodeLong(longValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(longValueBin, 0, longValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            long? longv = AmqpCodec.DecodeLong(new ByteBuffer(new ArraySegment<byte>(longValueBin)));
            Assert.True(longv == longValue, "Long value is not equal.");

            // float
            AmqpCodec.EncodeFloat(floatValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(floatValueBin, 0, floatValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            float? floatv = AmqpCodec.DecodeFloat(new ByteBuffer(new ArraySegment<byte>(floatValueBin)));
            Assert.True(floatv == floatValue, "Float value is not equal.");

            // double
            AmqpCodec.EncodeDouble(doubleValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(doubleValueBin, 0, doubleValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            double? doublev = AmqpCodec.DecodeDouble(new ByteBuffer(new ArraySegment<byte>(doubleValueBin)));
            Assert.True(doublev == doubleValue, "Double value is not equal.");

            //decimal
            decimal? dec32 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal32ValueBin)));
            Assert.True(dec32.Value == decimal32Value, "Decimal32 value is not equal");

            decimal? dec64 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal64ValueBin)));
            Assert.True(dec64.Value == decimal64Value, "Decimal64 value is not equal");

            decimal? dec128 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal128ValueBin)));
            Assert.True(dec128.Value == decimal128Value, "Decimal128 value is not equal");

            // char
            AmqpCodec.EncodeChar(charValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(charValueBin, 0, charValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            char? charv = AmqpCodec.DecodeChar(new ByteBuffer(new ArraySegment<byte>(charValueBin)));
            Assert.True(charv == charValue, "Char value is not equal.");

            // timestamp
            AmqpCodec.EncodeTimeStamp(dtValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(dtValueBin, 0, dtValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            DateTime? dtv = AmqpCodec.DecodeTimeStamp(new ByteBuffer(new ArraySegment<byte>(dtValueBin)));
            Assert.True(dtv == dtValue.ToUniversalTime(), "UByte value is not equal.");

            // uuid
            AmqpCodec.EncodeUuid(uuidValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uuidValueBin, 0, uuidValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            Guid? uuidv = AmqpCodec.DecodeUuid(new ByteBuffer(new ArraySegment<byte>(uuidValueBin)));
            Assert.True(uuidv == uuidValue, "Uuid value is not equal.");

            // binary 8
            AmqpCodec.EncodeBinary(bin8Value, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(bin8ValueBin, 0, bin8ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ArraySegment<byte> bin8v = AmqpCodec.DecodeBinary(new ByteBuffer(new ArraySegment<byte>(bin8ValueBin)));
            EnsureEqual(bin8v.Array, bin8v.Offset, bin8v.Count, bin8Value.Array, bin8Value.Offset, bin8Value.Count);

            // binary 32
            AmqpCodec.EncodeBinary(bin32Value, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(bin32ValueBin, 0, bin32ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ArraySegment<byte> bin32v = AmqpCodec.DecodeBinary(new ByteBuffer(new ArraySegment<byte>(bin32ValueBin)));
            EnsureEqual(bin32v.Array, bin32v.Offset, bin32v.Count, bin32Value.Array, bin32Value.Offset, bin32Value.Count);

            // symbol 8
            AmqpCodec.EncodeSymbol(strValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(sym8ValueBin, 0, sym8ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            AmqpSymbol symbol8v = AmqpCodec.DecodeSymbol(new ByteBuffer(new ArraySegment<byte>(sym8ValueBin)));
            Assert.True(symbol8v.Value == strValue, "Symbol8 string value is not equal.");

            // symbol 32
            AmqpSymbol symbol32v = AmqpCodec.DecodeSymbol(new ByteBuffer(new ArraySegment<byte>(sym32ValueBin)));
            Assert.True(symbol32v.Value == strValue, "Symbol32 string value is not equal.");

            // string 8 UTF8
            AmqpCodec.EncodeString(strValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(str8Utf8ValueBin, 0, str8Utf8ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            string str8Utf8 = AmqpCodec.DecodeString(new ByteBuffer(new ArraySegment<byte>(str8Utf8ValueBin)));
            Assert.True(str8Utf8 == strValue, "UTF8 string8 string value is not equal.");

            // string 32 UTF8
            string str32Utf8 = AmqpCodec.DecodeString(new ByteBuffer(new ArraySegment<byte>(str32Utf8ValueBin)));
            Assert.True(str32Utf8 == strValue, "UTF8 string32 string value is not equal.");
        }

        [Fact]
        public void AmqpCodecListTest()
        {
            byte[] workBuffer = new byte[4096];
            ByteBuffer buffer = new ByteBuffer(workBuffer);
            string strBig = new string('A', 512);

            List<object> list = new List<object>();
            list.Add(boolTrue);
            list.Add(boolFalse);
            list.Add(ubyteValue);
            list.Add(ushortValue);
            list.Add(uintValue);
            list.Add(ulongValue);
            list.Add(byteValue);
            list.Add(shortValue);
            list.Add(intValue);
            list.Add(longValue);
            list.Add(null);
            list.Add(floatValue);
            list.Add(doubleValue);
            list.Add(decimal32Value);
            list.Add(decimal64Value);
            list.Add(decimal128Value);
            list.Add(charValue);
            list.Add(dtValue);
            list.Add(uuidValue);
            list.Add(new ArraySegment<byte>());
            list.Add(bin8Value);
            list.Add(bin32Value);
            list.Add(new AmqpSymbol());
            list.Add(new AmqpSymbol(strValue));
            list.Add(new AmqpSymbol(strBig));
            list.Add(strValue);
            list.Add(strBig);
            list.Add(described1);
            list.Add(described2);
            list.Add(described3);
            list.Add(described4);

            AmqpCodec.EncodeList(list, buffer);

            // make sure the size written is correct (it has to be List32)
            // the first byte is FormatCode.List32
            int listSize = (int)AmqpBitConverter.ReadUInt(workBuffer, 1, 4);
            Assert.Equal(buffer.Length - 5, listSize);

            IList decList = AmqpCodec.DecodeList(buffer);
            int index = 0;

            Assert.True(decList[index++].Equals(true), "Boolean true expected.");
            Assert.True(decList[index++].Equals(false), "Boolean false expected.");
            Assert.True(decList[index++].Equals(ubyteValue), "UByte value not equal.");
            Assert.True(decList[index++].Equals(ushortValue), "UShort value not equal.");
            Assert.True(decList[index++].Equals(uintValue), "UInt value not equal.");
            Assert.True(decList[index++].Equals(ulongValue), "ULong value not equal.");
            Assert.True(decList[index++].Equals(byteValue), "Byte value not equal.");
            Assert.True(decList[index++].Equals(shortValue), "Short value not equal.");
            Assert.True(decList[index++].Equals(intValue), "Int value not equal.");
            Assert.True(decList[index++].Equals(longValue), "Long value not equal.");
            Assert.True(decList[index++] == null, "Null object expected.");
            Assert.True(decList[index++].Equals(floatValue), "Float value not equal.");
            Assert.True(decList[index++].Equals(doubleValue), "Double value not equal.");
            Assert.True(decList[index++].Equals(decimal32Value), "Decimal32 value not equal.");
            Assert.True(decList[index++].Equals(decimal64Value), "Decimal64 value not equal.");
            Assert.True(decList[index++].Equals(decimal128Value), "Decimal128 value not equal.");
            Assert.True(decList[index++].Equals(charValue), "Char value not equal.");
            Assert.True(decList[index++].Equals(dtValue), "TimeStamp value not equal.");
            Assert.True(decList[index++].Equals(uuidValue), "Uuid value not equal.");

            Assert.True(decList[index++] == null, "Null binary expected.");
            ArraySegment<byte> bin8 = (ArraySegment<byte>)decList[index++];
            EnsureEqual(bin8.Array, bin8.Offset, bin8.Count, bin8Value.Array, bin8Value.Offset, bin8Value.Count);
            ArraySegment<byte> bin32 = (ArraySegment<byte>)decList[index++];
            EnsureEqual(bin32.Array, bin32.Offset, bin32.Count, bin32Value.Array, bin32Value.Offset, bin32Value.Count);

            Assert.True(decList[index++] == null, "Null symbol expected.");
            AmqpSymbol symDecode = (AmqpSymbol)decList[index++];
            Assert.True(symDecode.Equals(strValue), "AmqpSymbol value not equal.");
            symDecode = (AmqpSymbol)decList[index++];
            Assert.True(symDecode.Equals(strBig), "AmqpSymbol value (big) not equal.");

            string strDecode = (string)decList[index++];
            Assert.True(strDecode.Equals(strValue), "string value not equal.");
            strDecode = (string)decList[index++];
            Assert.True(strDecode.Equals(strBig), "string value (big) not equal.");

            DescribedType described = (DescribedType)decList[index++];
            Assert.True(described.Descriptor.Equals(described1.Descriptor), "Described value 1 descriptor is different");
            Assert.True(described.Value.Equals(described1.Value), "Described value 1 value is different");
            described = (DescribedType)decList[index++];
            Assert.True(described.Descriptor.Equals(described2.Descriptor), "Described value 2 descriptor is different");
            Assert.True(described.Value.Equals(described2.Value), "Described value 2 value is different");
            described = (DescribedType)decList[index++];
            Assert.True(described.Descriptor.Equals(described3.Descriptor), "Described value 3 descriptor is different");
            Assert.True(described.Value.Equals(described3.Value), "Described value 3 value is different");
            described = (DescribedType)decList[index++];
            EnsureEqual((DateTime)described4.Descriptor, (DateTime)described.Descriptor);
            EnsureEqual((IList)described.Value, (IList)described4.Value);
        }

        [Fact]
        public void AmqpCodecList0Test()
        {
            byte[] list0Bin = new byte[] { 0x45 };
            byte[] workBuffer = new byte[128];
            ByteBuffer buffer = new ByteBuffer(workBuffer);

            List<object> list0 = new List<object>();
            AmqpCodec.EncodeList(list0, buffer);
            EnsureEqual(list0Bin, 0, list0Bin.Length, buffer.Buffer, buffer.Offset, buffer.Length);

            IList list0v = AmqpCodec.DecodeList(buffer);
            Assert.True(list0v.Count == 0, "The list should contain 0 items.");
        }

        [Fact]
        public void AmqpCodecMapTest()
        {
            byte[] workBuffer = new byte[4096];
            ByteBuffer buffer = new ByteBuffer(workBuffer);
            string strBig = new string('A', 512);

            AmqpMap map = new AmqpMap();
            map.Add(new MapKey("boolTrue"), boolTrue);
            map.Add(new MapKey("boolFalse"), boolFalse);
            map.Add(new MapKey("ubyte"), ubyteValue);
            map.Add(new MapKey("ushort"), ushortValue);
            map.Add(new MapKey("uint"), uintValue);
            map.Add(new MapKey("ulong"), ulongValue);
            map.Add(new MapKey("byte"), byteValue);
            map.Add(new MapKey("short"), shortValue);
            map.Add(new MapKey("int"), intValue);
            map.Add(new MapKey("long"), longValue);
            map.Add(new MapKey("null"), null);
            map.Add(new MapKey("float"), floatValue);
            map.Add(new MapKey("double"), doubleValue);
            map.Add(new MapKey("decimal32"), decimal32Value);
            map.Add(new MapKey("decimal64"), decimal64Value);
            map.Add(new MapKey("decimal128"), decimal128Value);
            map.Add(new MapKey("char"), charValue);
            map.Add(new MapKey("datetime"), dtValue);
            map.Add(new MapKey("uuid"), uuidValue);
            map.Add(new MapKey("binaryNull"), new ArraySegment<byte>());
            map.Add(new MapKey("binary8"), bin8Value);
            map.Add(new MapKey("binary32"), bin32Value);
            map.Add(new MapKey("symbolNull"), new AmqpSymbol());
            map.Add(new MapKey("symbol8"), new AmqpSymbol(strValue));
            map.Add(new MapKey("symbol32"), new AmqpSymbol(strBig));
            map.Add(new MapKey("string8"), strValue);
            map.Add(new MapKey("string32"), strBig);
            map.Add(new MapKey("described1"), described1);

            AmqpCodec.EncodeMap(map, buffer);

            // make sure the size written is correct (it has to be Map32)
            // the first byte is FormatCode.Map32
            int mapSize = (int)AmqpBitConverter.ReadUInt(workBuffer, 1, 4);
            Assert.Equal(buffer.Length - 5, mapSize);

            AmqpMap decMap = AmqpCodec.DecodeMap(buffer);

            Assert.True(decMap[new MapKey("boolTrue")].Equals(true), "Boolean true expected.");
            Assert.True(decMap[new MapKey("boolFalse")].Equals(false), "Boolean false expected.");
            Assert.True(decMap[new MapKey("ubyte")].Equals(ubyteValue), "UByte value not equal.");
            Assert.True(decMap[new MapKey("ushort")].Equals(ushortValue), "UShort value not equal.");
            Assert.True(decMap[new MapKey("uint")].Equals(uintValue), "UInt value not equal.");
            Assert.True(decMap[new MapKey("ulong")].Equals(ulongValue), "ULong value not equal.");
            Assert.True(decMap[new MapKey("byte")].Equals(byteValue), "Byte value not equal.");
            Assert.True(decMap[new MapKey("short")].Equals(shortValue), "Short value not equal.");
            Assert.True(decMap[new MapKey("int")].Equals(intValue), "Int value not equal.");
            Assert.True(decMap[new MapKey("long")].Equals(longValue), "Long value not equal.");
            Assert.True(decMap[new MapKey("null")] == null, "Null object expected.");
            Assert.True(decMap[new MapKey("float")].Equals(floatValue), "Float value not equal.");
            Assert.True(decMap[new MapKey("double")].Equals(doubleValue), "Double value not equal.");
            Assert.True(decMap[new MapKey("decimal32")].Equals(decimal32Value), "Decimal32 value not equal.");
            Assert.True(decMap[new MapKey("decimal64")].Equals(decimal64Value), "Decimal64 value not equal.");
            Assert.True(decMap[new MapKey("decimal128")].Equals(decimal128Value), "Decimal128 value not equal.");
            Assert.True(decMap[new MapKey("char")].Equals(charValue), "Char value not equal.");
            Assert.True(decMap[new MapKey("datetime")].Equals(dtValue), "TimeStamp value not equal.");
            Assert.True(decMap[new MapKey("uuid")].Equals(uuidValue), "Uuid value not equal.");
            Assert.True(decMap[new MapKey("binaryNull")] == null, "Null binary expected.");
            ArraySegment<byte> bin8 = (ArraySegment<byte>)decMap[new MapKey("binary8")];
            EnsureEqual(bin8.Array, bin8.Offset, bin8.Count, bin8Value.Array, bin8Value.Offset, bin8Value.Count);
            ArraySegment<byte> bin32 = (ArraySegment<byte>)decMap[new MapKey("binary32")];
            EnsureEqual(bin32.Array, bin32.Offset, bin32.Count, bin32Value.Array, bin32Value.Offset, bin32Value.Count);

            Assert.True(decMap[new MapKey("symbolNull")] == null, "Null symbol expected.");
            AmqpSymbol symDecode = (AmqpSymbol)decMap[new MapKey("symbol8")];
            Assert.True(symDecode.Equals(strValue), "AmqpSymbol value not equal.");
            symDecode = (AmqpSymbol)decMap[new MapKey("symbol32")];
            Assert.True(symDecode.Equals(strBig), "AmqpSymbol value (big) not equal.");

            string strDecode = (string)decMap[new MapKey("string8")];
            Assert.True(strDecode.Equals(strValue), "string value not equal.");
            strDecode = (string)decMap[new MapKey("string32")];
            Assert.True(strDecode.Equals(strBig), "string value (big) not equal.");

            DescribedType described = (DescribedType)decMap[new MapKey("described1")];
            Assert.True(described.Descriptor.Equals(described1.Descriptor), "Described value 1 descriptor is different");
            Assert.True(described.Value.Equals(described1.Value), "Described value 1 value is different");
        }

        [Fact]
        public void AmqpCodecMultipleTest()
        {
            byte[] workBuffer = new byte[2048];
            ByteBuffer buffer = new ByteBuffer(workBuffer);

            Multiple<int> nullValue = null;
            Multiple<string> oneValue = new Multiple<string>(new string[] { strValue });
            Multiple<Guid> twoValues = new Multiple<Guid>(new Guid[] { uuidValue, uuidValue });
            Multiple<AmqpSymbol> threeValues = new Multiple<AmqpSymbol>(new AmqpSymbol[] { "sym1", "sym2", "sym3" });

            AmqpCodec.EncodeMultiple<int>(nullValue, buffer);
            AmqpCodec.EncodeMultiple<string>(oneValue, buffer);
            AmqpCodec.EncodeMultiple<Guid>(twoValues, buffer);
            AmqpCodec.EncodeMultiple<AmqpSymbol>(threeValues, buffer);

            Multiple<int> nullDecoded = AmqpCodec.DecodeMultiple<int>(buffer);
            Multiple<string> oneDecoded = AmqpCodec.DecodeMultiple<string>(buffer);
            Multiple<Guid> twoDecoded = AmqpCodec.DecodeMultiple<Guid>(buffer);
            Multiple<AmqpSymbol> threeDecoded = AmqpCodec.DecodeMultiple<AmqpSymbol>(buffer);

            Assert.True(nullDecoded == null, "the null multiple value is not null");
            Assert.True(Multiple<string>.Intersect(oneValue, oneDecoded).Count == 1, "multiple of one string value failed");
            Assert.True(Multiple<Guid>.Intersect(twoValues, twoDecoded).Count == 2, "multiple of two uuid values failed");
            Assert.True(Multiple<AmqpSymbol>.Intersect(threeValues, threeDecoded).Count == 3, "multiple of three symbol values failed");
        }

        [Fact]
        public void AmqpCodecFramingTypeTest()
        {
            byte[] workBuffer = new byte[1024 * 16];

            ByteBuffer buffer = new ByteBuffer(workBuffer);

            // transport
            EncodeDescribedList(buffer, Open.Code, "my-container", "my-hostname", (uint)1000);
            EncodeDescribedList(buffer, Begin.Name, (ushort)100, (uint)200, (uint)300, (uint)400);
            EncodeDescribedList(buffer, Attach.Code, "my-link", (uint)100, false, null, null);
            EncodeDescribedList(buffer, Flow.Code, null, (uint)100, (uint)200, (uint)300, null, (uint)400);
            EncodeDescribedList(buffer, Transfer.Name, (uint)100, null, null, (uint)0);
            EncodeDescribedList(buffer, Disposition.Name, true, (uint)200);
            EncodeDescribedList(buffer, Detach.Code, (uint)300);
            EncodeDescribedList(buffer, End.Code, new object[0]);
            EncodeDescribedList(buffer, Close.Code, null, null, null, null, "this string should be ignored");
            EncodeDescribedList(buffer, Error.Name, AmqpErrorCode.IllegalState);
            EncodeDescribedList(buffer, Received.Code, (uint)0, (ulong)12);
            EncodeDescribedList(buffer, Accepted.Name, "this string should be ignored.");
            EncodeDescribedList(buffer, Rejected.Code, null, "this string should be ignored", 100);
            EncodeDescribedList(buffer, Released.Code, new object[0]);
            EncodeDescribedList(buffer, Modified.Code, true, false, new AmqpMap(), null);
            EncodeDescribedList(buffer, Source.Name, "my-address");
            EncodeDescribedList(buffer, Target.Code, "my-target", (uint)TerminusDurability.Configuration, TerminusExpiryPolicy.LinkDetach, (uint)100);
            EncodeDescribedList(buffer, DeleteOnClose.Code, null);
            EncodeDescribedList(buffer, DeleteOnNoLinks.Name, null);
            EncodeDescribedList(buffer, DeleteOnNoMessages.Name, null, null);
            EncodeDescribedList(buffer, DeleteOnNoLinksOrMessages.Code, new object[0]);
            // transaction
            EncodeDescribedList(buffer, Coordinator.Code, null);
            EncodeDescribedList(buffer, Declare.Name, null, null);
            EncodeDescribedList(buffer, Discharge.Name, bin8Value, false, null);
            EncodeDescribedList(buffer, Declared.Code, bin8Value);
            EncodeDescribedList(buffer, TransactionalState.Code, bin8Value, null, null);
            // sasl
            EncodeDescribedList(buffer, SaslMechanisms.Code, new AmqpSymbol[] { "plain", "kerb" });
            EncodeDescribedList(buffer, SaslInit.Name, (AmqpSymbol)"plain", bin8Value, "sasl-host", null, 100);
            EncodeDescribedList(buffer, SaslChallenge.Name, bin8Value, (AmqpSymbol)"this symbol should be ignored");
            EncodeDescribedList(buffer, SaslResponse.Code, bin8Value);
            EncodeDescribedList(buffer, SaslOutcome.Code, (byte)SaslCode.SysPerm);

            // transport
            AmqpCodec.DecodeKnownType<Open>(buffer);
            AmqpCodec.DecodeKnownType<Begin>(buffer);
            AmqpCodec.DecodeKnownType<Attach>(buffer);
            AmqpCodec.DecodeKnownType<Flow>(buffer);
            AmqpCodec.DecodeKnownType<Transfer>(buffer);
            AmqpCodec.DecodeKnownType<Disposition>(buffer);
            AmqpCodec.DecodeKnownType<Detach>(buffer);
            AmqpCodec.DecodeKnownType<End>(buffer);
            AmqpCodec.DecodeKnownType<Close>(buffer);
            AmqpCodec.DecodeKnownType<Error>(buffer);
            AmqpCodec.DecodeKnownType<Received>(buffer);
            AmqpCodec.DecodeKnownType<Accepted>(buffer);
            AmqpCodec.DecodeKnownType<Rejected>(buffer);
            AmqpCodec.DecodeKnownType<Released>(buffer);
            AmqpCodec.DecodeKnownType<Modified>(buffer);
            AmqpCodec.DecodeKnownType<Source>(buffer);
            AmqpCodec.DecodeKnownType<Target>(buffer);
            AmqpCodec.DecodeKnownType<DeleteOnClose>(buffer);
            AmqpCodec.DecodeKnownType<DeleteOnNoLinks>(buffer);
            AmqpCodec.DecodeKnownType<DeleteOnNoMessages>(buffer);
            AmqpCodec.DecodeKnownType<DeleteOnNoLinksOrMessages>(buffer);
            //transaction
            AmqpCodec.DecodeKnownType<Coordinator>(buffer);
            AmqpCodec.DecodeKnownType<Declare>(buffer);
            AmqpCodec.DecodeKnownType<Discharge>(buffer);
            AmqpCodec.DecodeKnownType<Declared>(buffer);
            AmqpCodec.DecodeKnownType<TransactionalState>(buffer);
            //sasl
            AmqpCodec.DecodeKnownType<SaslMechanisms>(buffer);
            AmqpCodec.DecodeKnownType<SaslInit>(buffer);
            AmqpCodec.DecodeKnownType<SaslChallenge>(buffer);
            AmqpCodec.DecodeKnownType<SaslResponse>(buffer);
            AmqpCodec.DecodeKnownType<SaslOutcome>(buffer);

            Assert.True(buffer.Length == 0, "All bytes in the buffer should be consumed");
        }

        [Fact]
        public void AmqpCodecDescribedArrayTest()
        {
            int size = AmqpCodec.GetObjectEncodeSize(described5);
            ByteBuffer buffer = new ByteBuffer(new byte[size]);
            AmqpCodec.EncodeObject(described5, buffer);
            DescribedType decoded = (DescribedType)AmqpCodec.DecodeObject(buffer);
            Assert.True(decoded.Descriptor.Equals(described5.Descriptor), "Descriptor value not equal");
            string[] original = (string[])described5.Value;
            string[] array = (string[])decoded.Value;
            Assert.True(original.Length == array.Length, string.Format("length not equal {0} != {1}", original.Length, array.Length));
            for (int i = 0; i < original.Length; ++i)
            {
                Assert.True(original[i] == array[i], string.Format("index {0}: {1} != {2}", i, original[i], array[i]));
            }
        }

        [Fact]
        public void AmqpCodecArrayBoolTest()
        {
            ArrayTest<bool>(new bool[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<bool>(new bool[] { true }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<bool>(new bool[] { true, false, false, true, false, false, true }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayByteTest()
        {
            ArrayTest<byte>(new byte[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<byte>(new byte[] { 128 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<byte>(new byte[] { 0, 1, 2, 3, 4, 200, 255 }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArraySByteTest()
        {
            ArrayTest<sbyte>(new sbyte[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<sbyte>(new sbyte[] { -1 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<sbyte>(new sbyte[] { -127, -9, 0, 9, 127 }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayUShortTest()
        {
            ArrayTest<ushort>(new ushort[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<ushort>(new ushort[] { 1234 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<ushort>(new ushort[] { 0, 1, 2, 0x1234, 0xab00, 0xffff }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayShortTest()
        {
            ArrayTest<short>(new short[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<short>(new short[] { -1 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<short>(new short[] { short.MinValue, -127, -9, 0, 9, 127, short.MaxValue }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayUIntTest()
        {
            ArrayTest<uint>(new uint[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<uint>(new uint[] { 42 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<uint>(new uint[] { 0, 1, 2, 0x1234, 0xab00, 0xffff, 0x239d9e, 0xffffffff }, (n1, n2) => Assert.Equal(n1, n2));
            // Array with uint0 value (0) mixed with larger values
            ArrayTest<uint>(new uint[] { 0, 0xffffffff, 0, 42, 0 }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayIntTest()
        {
            ArrayTest<int>(new int[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<int>(new int[] { 0 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<int>(new int[] { int.MinValue, short.MinValue, -127, -9, 0, 9, 127, short.MaxValue, int.MaxValue }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayULongTest()
        {
            ArrayTest<ulong>(new ulong[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<ulong>(new ulong[] { 42 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<ulong>(new ulong[] { 0, 1, 2, 0x1234, 0xab00, 0xffff, 0x239d9e, 0xffffffff, 0x329999999, 0xffffffffffffffff }, (n1, n2) => Assert.Equal(n1, n2));
            // Array with ulong0 value (0) mixed with larger values
            ArrayTest<ulong>(new ulong[] { 0, 0xffffffffffffffff, 0, 42, 0 }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayLongTest()
        {
            ArrayTest<long>(new long[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<long>(new long[] { 0 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<long>(new long[] { long.MinValue, int.MinValue, short.MinValue, -127, -9, 0, 9, 127, short.MaxValue, int.MaxValue, long.MaxValue }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayFloatTest()
        {
            ArrayTest<float>(new float[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<float>(new float[] { 0f }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<float>(new float[] { float.MinValue, -238.233453f, 0, 89234.92394f, float.MaxValue }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayDoubleTest()
        {
            ArrayTest<double>(new double[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<double>(new double[] { 3.14 }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<double>(new double[] { double.MinValue, float.MinValue, -238.233453f, 0, 89234.92394f, float.MaxValue, double.MaxValue }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayCharTest()
        {
            ArrayTest<char>(new char[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<char>(new char[] { 'Z' }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<char>(new char[] { 'a', 'b', 'A', 'C' }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayDateTimeTest()
        {
            ArrayTest<DateTime>(new DateTime[0], (n1, n2) => EnsureEqual(n1, n2));
            ArrayTest<DateTime>(new DateTime[] { DateTime.UtcNow }, (n1, n2) => EnsureEqual(n1, n2));
            ArrayTest<DateTime>(
                new DateTime[] { DateTime.Now - TimeSpan.FromDays(100), DateTime.Now, DateTime.Now + TimeSpan.FromDays(100) },
                (n1, n2) => EnsureEqual(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayGuidTest()
        {
            ArrayTest<Guid>(new Guid[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<Guid>(new Guid[] { Guid.NewGuid() }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<Guid>(new Guid[] { Guid.NewGuid(), Guid.NewGuid(), Guid.NewGuid(), Guid.NewGuid() }, (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayBinaryTest()
        {
            ArrayTest<ArraySegment<byte>>(new ArraySegment<byte>[0], (n1, n2) => Assert.Equal(n1.Count, n2.Count));
            ArrayTest<ArraySegment<byte>>(
                new ArraySegment<byte>[] { new ArraySegment<byte>(new byte[] { 1, 2, 3 }) },
                (n1, n2) => Assert.Equal(n1.Count, n2.Count));
            // Mixed bin8 and bin32 sizes
            ArrayTest<ArraySegment<byte>>(
                new ArraySegment<byte>[] { bin8Value, bin32Value, new ArraySegment<byte>(new byte[0]) },
                (n1, n2) => Assert.Equal(n1.Count, n2.Count));
        }

        [Fact]
        public void AmqpCodecArrayStringTest()
        {
            ArrayTest<string>(new string[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<string>(new string[] { "hello" }, (n1, n2) => Assert.Equal(n1, n2));
            // Mixed str8 (<256 bytes) and str32 (>=256 bytes) strings
            ArrayTest<string>(
                new string[] { new string('A', 10), new string('B', 300), new string('C', 100) },
                (n1, n2) => Assert.Equal(n1, n2));
            // All str32 strings
            ArrayTest<string>(
                new string[] { new string('X', 300), new string('Y', 500) },
                (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArraySymbolTest()
        {
            ArrayTest<AmqpSymbol>(new AmqpSymbol[0], (n1, n2) => Assert.True(n1.Equals(n2)));
            ArrayTest<AmqpSymbol>(new AmqpSymbol[] { "sym1" }, (n1, n2) => Assert.True(n1.Equals(n2)));
            // Mixed sym8 and sym32 sizes
            ArrayTest<AmqpSymbol>(
                new AmqpSymbol[] { new string('A', 10), new string('B', 300), new string('C', 100) },
                (n1, n2) => Assert.True(n1.Equals(n2)));
        }

        [Fact]
        public void AmqpCodecArrayListTest()
        {
            ArrayTest<IList>(new IList[0], (n1, n2) => { });
            Guid uuid = Guid.NewGuid();
            List<object> list1 = new List<object> { str32Value, new AmqpSymbol(strValue), uuid, 8.88d };
            List<object> list2 = new List<object> { strValue, 3333u };
            ArrayTest<IList>(new IList[] { list1 }, (n1, n2) => EnsureEqual(n1, n2));
            ArrayTest<IList>(new IList[] { list1, list2, list1, list2 }, (n1, n2) => EnsureEqual(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayDescribedTypeTest()
        {
            ArrayTest<DescribedType>(new DescribedType[] { described4 }, (n1, n2) => { });
            ArrayTest<DescribedType>(
                new DescribedType[] { described4, described4, described4, described4 },
                (n1, n2) => { });
        }

        [Fact]
        public void AmqpCodecArrayDecimalTest()
        {
            ArrayTest<decimal>(new decimal[0], (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<decimal>(new decimal[] { 0m }, (n1, n2) => Assert.Equal(n1, n2));
            ArrayTest<decimal>(
                new decimal[] { decimal.MinValue, -234934.092348m, 0, 38743947394.2349324m, decimal.MaxValue },
                (n1, n2) => Assert.Equal(n1, n2));
        }

        [Fact]
        public void AmqpCodecArrayDecodeBooleanTrueTest()
        {
            ArrayDecodeTest<bool>(FormatCode.BooleanTrue, new byte[0], 0, new bool[0]);
            // BooleanTrue (0x41): zero-width, all elements are true
            ArrayDecodeTest<bool>(FormatCode.BooleanTrue, new byte[0], 3, new bool[] { true, true, true });
        }

        [Fact]
        public void AmqpCodecArrayDecodeBooleanFalseTest()
        {
            ArrayDecodeTest<bool>(FormatCode.BooleanFalse, new byte[0], 0, new bool[0]);
            // BooleanFalse (0x42): zero-width, all elements are false
            ArrayDecodeTest<bool>(FormatCode.BooleanFalse, new byte[0], 3, new bool[] { false, false, false });
        }

        [Fact]
        public void AmqpCodecArrayDecodeUInt0Test()
        {
            ArrayDecodeTest<uint>(FormatCode.UInt0, new byte[0], 0, new uint[0]);
            // UInt0 (0x43): zero-width, all elements are 0
            ArrayDecodeTest<uint>(FormatCode.UInt0, new byte[0], 3, new uint[] { 0, 0, 0 });
        }

        [Fact]
        public void AmqpCodecArrayDecodeSmallUIntTest()
        {
            ArrayDecodeTest<uint>(FormatCode.SmallUInt, new byte[0], 0, new uint[0]);
            // SmallUInt (0x52): 1 byte per element
            ArrayDecodeTest<uint>(FormatCode.SmallUInt, new byte[] { 0, 42, 255 }, 3, new uint[] { 0, 42, 255 });
        }

        [Fact]
        public void AmqpCodecArrayDecodeULong0Test()
        {
            ArrayDecodeTest<ulong>(FormatCode.ULong0, new byte[0], 0, new ulong[0]);
            // ULong0 (0x44): zero-width, all elements are 0
            ArrayDecodeTest<ulong>(FormatCode.ULong0, new byte[0], 3, new ulong[] { 0, 0, 0 });
        }

        [Fact]
        public void AmqpCodecArrayDecodeSmallULongTest()
        {
            ArrayDecodeTest<ulong>(FormatCode.SmallULong, new byte[0], 0, new ulong[0]);
            // SmallULong (0x53): 1 byte per element
            ArrayDecodeTest<ulong>(FormatCode.SmallULong, new byte[] { 0, 42, 255 }, 3, new ulong[] { 0, 42, 255 });
        }

        [Fact]
        public void AmqpCodecArrayDecodeSmallIntTest()
        {
            ArrayDecodeTest<int>(FormatCode.SmallInt, new byte[0], 0, new int[0]);
            // SmallInt (0x54): 1 signed byte per element
            ArrayDecodeTest<int>(FormatCode.SmallInt, new byte[] { 0xFF, 0x00, 0x7F }, 3, new int[] { -1, 0, 127 });
        }

        [Fact]
        public void AmqpCodecArrayDecodeSmallLongTest()
        {
            ArrayDecodeTest<long>(FormatCode.SmallLong, new byte[0], 0, new long[0]);
            // SmallLong (0x55): 1 signed byte per element
            ArrayDecodeTest<long>(FormatCode.SmallLong, new byte[] { 0xFF, 0x00, 0x7F }, 3, new long[] { -1, 0, 127 });
        }

        [Fact]
        public void AmqpSerializerListEncodingTest()
        {
            Action<Person, Person> personValidator = (p1, p2) =>
            {
                Assert.NotNull(p2);
                Assert.True(21 == p2.Age, "Age should be increased by OnDeserialized");
                Assert.Equal(p1.GetType().Name, p2.GetType().Name);
                Assert.Equal(p1.DateOfBirth.Value, p2.DateOfBirth.Value);
                Assert.Equal(p1.Properties.Count, p2.Properties.Count);
                foreach (var k in p1.Properties.Keys)
                {
                    Assert.Equal(p1.Properties[k], p2.Properties[k]);
                }
            };

            Action<List<int>, List<int>> gradesValidator = (l1, l2) =>
            {
                if (l1 == null || l2 == null)
                {
                    Assert.True(l1 == null && l2 == null);
                    return;
                }

                Assert.Equal(l1.Count, l2.Count);
                for (int i = 0; i < l1.Count; ++i)
                {
                    Assert.Equal(l1[i], l2[i]);
                }
            };

            // Create an object to be serialized
            Person p = new Student("Tom")
                {
                    Address = new Address() { FullAddress = new string('B', 1024) }, 
                    Grades = new List<int>() { 1, 2, 3, 4, 5 }
                };
            p.Age = 20;
            p.DateOfBirth = new DateTime(1980, 5, 12, 10, 2, 45, DateTimeKind.Utc);
            p.Properties.Add("height", 6.1);
            p.Properties.Add("male", true);
            p.Properties.Add("nick-name", "big foot");

            var stream = new MemoryStream(new byte[4096], 0, 4096, true, true);
            AmqpContractSerializer.WriteObject(stream, p);
            stream.Flush();
            
            // Deserialize and verify
            stream.Seek(0, SeekOrigin.Begin);
            Person p3 = AmqpContractSerializer.ReadObject<Person>(stream);
            personValidator(p, p3);
            Assert.Equal(((Student)p).Address.FullAddress, ((Student)p3).Address.FullAddress);
            gradesValidator(((Student)p).Grades, ((Student)p3).Grades);

            // Inter-op: it should be an AMQP described list as other clients see it
            stream.Seek(0, SeekOrigin.Begin);
            DescribedType dl1 = (DescribedType)AmqpEncoding.DecodeObject(new ByteBuffer(stream.ToArray(), 0, (int)stream.Length));
            Assert.Equal(1ul, dl1.Descriptor);
            List<object> lv = dl1.Value as List<object>;
            Assert.NotNull(lv);
            Assert.Equal(p.Name, lv[0]);
            Assert.Equal(p.Age, lv[1]);
            Assert.Equal(p.DateOfBirth.Value, lv[2]);
            Assert.True(lv[3] is DescribedType, "Address is decribed type");
            Assert.Equal(3ul, ((DescribedType)lv[3]).Descriptor);
            Assert.Equal(((List<object>)((DescribedType)lv[3]).Value)[0], ((Student)p).Address.FullAddress);
            Assert.True(lv[4] is AmqpMap, "Properties should be map");
            Assert.Equal(((AmqpMap)lv[4])[new MapKey("height")], p.Properties["height"]);
            Assert.Equal(((AmqpMap)lv[4])[new MapKey("male")], p.Properties["male"]);
            Assert.Equal(((AmqpMap)lv[4])[new MapKey("nick-name")], p.Properties["nick-name"]);
            Assert.True(lv[5] is List<object>);

            // Non-default serializer
            AmqpContractSerializer serializer = new AmqpContractSerializer();
            ByteBuffer bf1 = new ByteBuffer(1024, true);
            serializer.WriteObjectInternal(bf1, p);

            Person p4 = serializer.ReadObjectInternal<Person, Person>(bf1);
            personValidator(p, p4);

            // Extensible: more items in the payload should not break
            DescribedType dl2 = new DescribedType(
                new AmqpSymbol("teacher"),
                new List<object>() { "Jerry", 40, null, 50000, lv[4], null, null, "unknown-string", true, new AmqpSymbol("unknown-symbol")});
            ByteBuffer bf2 = new ByteBuffer(1024, true);
            AmqpEncoding.EncodeObject(dl2, bf2);
            AmqpCodec.EncodeULong(100ul, bf2);

            Person p5 = serializer.ReadObjectInternal<Person, Person>(bf2);
            Assert.True(p5 is Teacher);
            Assert.Equal(100ul, AmqpCodec.DecodeULong(bf2));   // unknowns should be skipped
            Assert.Equal(0, bf2.Length);

            // teacher
            Teacher teacher = new Teacher("Han");
            teacher.Age = 30;
            teacher.Sallary = 60000;
            teacher.Classes = new Dictionary<int, string>() { { 101, "CS" }, { 102, "Math" }, { 205, "Project" } };

            ByteBuffer bf3 = new ByteBuffer(1024, true);
            serializer.WriteObjectInternal(bf3, teacher);

            Person p6 = serializer.ReadObjectInternal<Person, Person>(bf3);
            Assert.True(p6 is Teacher);
            Assert.Equal(teacher.Age + 1, p6.Age);
            Assert.Equal(teacher.Sallary * 2, ((Teacher)p6).Sallary);
            Assert.Equal(teacher.Id, ((Teacher)p6).Id);
            Assert.Equal(teacher.Classes[101], ((Teacher)p6).Classes[101]);
            Assert.Equal(teacher.Classes[102], ((Teacher)p6).Classes[102]);
            Assert.Equal(teacher.Classes[205], ((Teacher)p6).Classes[205]);
        }

        [Fact]
        public void AmqpSerializerMapEncodingTest()
        {
            NamedList<string> list = new NamedList<string>()
            {
                Name = "test-list",
                List = new string[] { "v1", "v2" }
            };

            AmqpContractSerializer serializer = new AmqpContractSerializer();
            ByteBuffer b = new ByteBuffer(1024, true);
            serializer.WriteObjectInternal(b, list);

            var result = serializer.ReadObjectInternal<NamedList<string>, NamedList<string>>(b);
            Assert.Equal(list.Name, result.Name);
            EnsureEqual((IList)list.List, (IList)result.List);
        }

#if !NETCOREAPP
        [Fact]
        public void AmqpExceptionSerializeTest()
        {
            const string errorDescription = "No link found...";
            var amqpException1 = new AmqpException(AmqpErrorCode.NotFound, errorDescription);

            IFormatter formatter = new NetDataContractSerializer();
            using (MemoryStream memoryStream = new MemoryStream())
            {
                formatter.Serialize(memoryStream, amqpException1);
                memoryStream.Position = 0;
                AmqpException amqpException2 = (AmqpException)formatter.Deserialize(memoryStream);
                Assert.False(object.ReferenceEquals(amqpException1, amqpException2), "Exceptions should not be the same instance!");
                Assert.Equal(amqpException1.Message, amqpException2.Message);
                Assert.Equal(amqpException1.Error.Condition, amqpException2.Error.Condition);
                Assert.Equal(amqpException1.Error.Description, amqpException2.Error.Description);
            }
        }
#endif

        static void EncodeDescribedList(ByteBuffer buffer, object descriptor, params object[] values)
        {
            object descriptor2 = descriptor is string ? (AmqpSymbol)(string)descriptor : descriptor;
            object[] values2 = values ?? new object[] { null };

            DescribedType describedType = new DescribedType(descriptor2, new List<object>(values2));
            AmqpEncoding.EncodeObject(describedType, buffer);
        }

        static void ArrayTest<T>(T[] array, Action<T, T> validate)
        {
            byte[] workBuffer = new byte[4096];
            ByteBuffer buffer;
            EncodingBase arrayEncoding = AmqpEncoding.GetEncoding(FormatCode.Array32);

            // Path 1: static Encode<T> → static Decode<T>
            AmqpCodec.EncodeArray(array, buffer = new ByteBuffer(workBuffer));
            ValidateArraySizeField(buffer);
            T[] decodedArray = AmqpCodec.DecodeArray<T>(buffer);
            Assert.Equal(array.Length, decodedArray.Length);
            for (int i = 0; i < decodedArray.Length; ++i)
            {
                validate(array[i], decodedArray[i]);
            }

            // Path 2: static Encode<T> → instance DecodeObject
            AmqpCodec.EncodeArray(array, buffer = new ByteBuffer(workBuffer));
            ValidateArraySizeField(buffer);
            decodedArray = (T[])arrayEncoding.DecodeObject(buffer, 0);
            Assert.Equal(array.Length, decodedArray.Length);
            for (int i = 0; i < decodedArray.Length; ++i)
            {
                validate(array[i], decodedArray[i]);
            }

            // Path 3: instance EncodeObject → static Decode<T>
            buffer = new ByteBuffer(workBuffer);
            arrayEncoding.EncodeObject(array, false, buffer);
            ValidateArraySizeField(buffer);
            decodedArray = AmqpCodec.DecodeArray<T>(buffer);
            Assert.Equal(array.Length, decodedArray.Length);
            for (int i = 0; i < decodedArray.Length; ++i)
            {
                validate(array[i], decodedArray[i]);
            }

            // Path 4: instance EncodeObject → instance DecodeObject
            buffer = new ByteBuffer(workBuffer);
            arrayEncoding.EncodeObject(array, false, buffer);
            ValidateArraySizeField(buffer);
            decodedArray = (T[])arrayEncoding.DecodeObject(buffer, 0);
            Assert.Equal(array.Length, decodedArray.Length);
            for (int i = 0; i < decodedArray.Length; ++i)
            {
                validate(array[i], decodedArray[i]);
            }
        }

        // Validates the size field in an encoded Array32: size should equal total length - FC(1) - sizeField(4).
        static void ValidateArraySizeField(ByteBuffer buffer)
        {
            byte[] raw = buffer.Buffer;
            int start = buffer.Offset;
            Assert.True(raw[start] == FormatCode.Array32, "Expected Array32 format code.");
            uint size = (uint)(raw[start + 1] << 24 | raw[start + 2] << 16 | raw[start + 3] << 8 | raw[start + 4]);
            Assert.Equal((uint)(buffer.Length - 1 - 4), size);
        }

        // Builds a raw AMQP array8 byte sequence and decodes it, verifying against expected values.
        static void ArrayDecodeTest<T>(byte elementFormatCode, byte[] valueBytes, int count, T[] expected)
        {
            // Wire format: Array8(1) | size(1) | count(1) | elementFC(1) | valueBytes
            int size = 1 + 1 + valueBytes.Length; // count(1) + element FC(1) + values
            byte[] raw = new byte[1 + 1 + size]; // Array8 FC + size field + payload
            int offset = 0;
            raw[offset++] = FormatCode.Array8;
            raw[offset++] = (byte)size;
            raw[offset++] = (byte)count;
            raw[offset++] = elementFormatCode;
            Array.Copy(valueBytes, 0, raw, offset, valueBytes.Length);

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(raw));
            T[] decoded = AmqpCodec.DecodeArray<T>(buffer);
            Assert.Equal(expected.Length, decoded.Length);
            for (int i = 0; i < expected.Length; i++)
            {
                Assert.Equal(expected[i], decoded[i]);
            }
        }

        void EnsureEqual(byte[] data1, int offset1, int count1, byte[] data2, int offset2, int count2)
        {
            Assert.True(count1 == count2, "Count is not equal.");
            for (int i = 0; i < count1; ++i)
            {
                byte b1 = data1[offset1 + i];
                byte b2 = data2[offset2 + i];
                Assert.True(b1 == b2, string.Format("The {0}th byte is not equal ({1} != {2}).", i, b1, b2));
            }
        }

        void EnsureEqual(IList list1, IList list2)
        {
            if (list1 == null && list2 == null)
            {
                return;
            }

            Assert.True(list1 != null && list2 != null, "One of the list is null");

            Assert.True(list1.Count == list2.Count, "Count not equal.");
            for (int i = 0; i < list1.Count; i++)
            {
                Assert.True(list1[i].Equals(list2[i]), "Value not equal.");
            }
        }

        void EnsureEqual(DateTime d1, DateTime d2)
        {
            Assert.True(Math.Abs((d1.ToUniversalTime() - d2.ToUniversalTime()).TotalMilliseconds) < 5, "Datetime difference is greater than 5ms.");
        }

        [Fact]
        public void AmqpCodecArrayCountBoundsCheckTest()
        {
            // Validates that an Array32 payload with COUNT exceeding the collection
            // limit throws AmqpException (decode error).
            RunAmqpCodecArrayCountBoundsCheckTest<bool>(FormatCode.Boolean);
            RunAmqpCodecArrayCountBoundsCheckTest<bool>(FormatCode.BooleanTrue);
            RunAmqpCodecArrayCountBoundsCheckTest<bool>(FormatCode.BooleanFalse);
            RunAmqpCodecArrayCountBoundsCheckTest<byte>(FormatCode.UByte);
            RunAmqpCodecArrayCountBoundsCheckTest<ushort>(FormatCode.UShort);
            RunAmqpCodecArrayCountBoundsCheckTest<uint>(FormatCode.UInt);
            RunAmqpCodecArrayCountBoundsCheckTest<uint>(FormatCode.SmallUInt);
            RunAmqpCodecArrayCountBoundsCheckTest<uint>(FormatCode.UInt0);
            RunAmqpCodecArrayCountBoundsCheckTest<ulong>(FormatCode.ULong);
            RunAmqpCodecArrayCountBoundsCheckTest<ulong>(FormatCode.SmallULong);
            RunAmqpCodecArrayCountBoundsCheckTest<ulong>(FormatCode.ULong0);
            RunAmqpCodecArrayCountBoundsCheckTest<sbyte>(FormatCode.Byte);
            RunAmqpCodecArrayCountBoundsCheckTest<short>(FormatCode.Short);
            RunAmqpCodecArrayCountBoundsCheckTest<int>(FormatCode.Int);
            RunAmqpCodecArrayCountBoundsCheckTest<int>(FormatCode.SmallInt);
            RunAmqpCodecArrayCountBoundsCheckTest<long>(FormatCode.Long);
            RunAmqpCodecArrayCountBoundsCheckTest<long>(FormatCode.SmallLong);
            RunAmqpCodecArrayCountBoundsCheckTest<float>(FormatCode.Float);
            RunAmqpCodecArrayCountBoundsCheckTest<double>(FormatCode.Double);
            RunAmqpCodecArrayCountBoundsCheckTest<char>(FormatCode.Char);
            RunAmqpCodecArrayCountBoundsCheckTest<DateTime>(FormatCode.TimeStamp);
            RunAmqpCodecArrayCountBoundsCheckTest<Guid>(FormatCode.Uuid);
            RunAmqpCodecArrayCountBoundsCheckTest<decimal>(FormatCode.Decimal128);
            RunAmqpCodecArrayCountBoundsCheckTest<ArraySegment<byte>>(FormatCode.Binary8);
            RunAmqpCodecArrayCountBoundsCheckTest<ArraySegment<byte>>(FormatCode.Binary32);
            RunAmqpCodecArrayCountBoundsCheckTest<string>(FormatCode.String8Utf8);
            RunAmqpCodecArrayCountBoundsCheckTest<string>(FormatCode.String32Utf8);
            RunAmqpCodecArrayCountBoundsCheckTest<AmqpSymbol>(FormatCode.Symbol8);
            RunAmqpCodecArrayCountBoundsCheckTest<AmqpSymbol>(FormatCode.Symbol32);
            RunAmqpCodecArrayCountBoundsCheckTest<IList>(FormatCode.List0);
            RunAmqpCodecArrayCountBoundsCheckTest<IList>(FormatCode.List8);
            RunAmqpCodecArrayCountBoundsCheckTest<IList>(FormatCode.List32);
            RunAmqpCodecArrayCountBoundsCheckTest<AmqpMap>(FormatCode.Map8);
            RunAmqpCodecArrayCountBoundsCheckTest<AmqpMap>(FormatCode.Map32);
            RunAmqpCodecArrayCountBoundsCheckTest<Array>(FormatCode.Array8);
            RunAmqpCodecArrayCountBoundsCheckTest<Array>(FormatCode.Array32);
            RunAmqpCodecArrayCountBoundsCheckTest<DescribedType>(FormatCode.Described);
        }

        static void RunAmqpCodecArrayCountBoundsCheckTest<T>(FormatCode formatCode)
        {
            byte[] payload = new byte[10];
            payload[0] = FormatCode.Array32;
            payload[1] = 0x00; payload[2] = 0x00; payload[3] = 0x00; payload[4] = 0x05;
            payload[5] = 0x7F; payload[6] = 0xFF; payload[7] = 0xFF; payload[8] = 0xFF;
            payload[9] = formatCode;

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                AmqpException ex = Assert.Throws<AmqpException>(() => AmqpEncoding.GetEncoding(FormatCode.Array32).DecodeObject(buffer, 0));
                Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeArray<T>(buffer));
                Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }
        }

        [Fact]
        public void AmqpCodecArrayCountLimitFrameTest()
        {
            byte[] reproFrame = new byte[]
            {
                0x00, 0x00, 0x00, 0x27, 0x02, 0x00, 0x00, 0x00,
                0x00, 0x53, 0x10, 0xd0, 0x00, 0x00, 0x00, 0x17,
                0x00, 0x00, 0x00, 0x08, 0xa1, 0x01, 0x78, 0x40,
                0x40, 0x40, 0x40, 0x40, 0x40, 0xf0, 0x00, 0x00,
                0x00, 0x09, 0x7f, 0xff, 0xff, 0xff, 0xa3
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(reproFrame));
            AmqpException ex = Assert.Throws<AmqpException>(() =>
            {
                Frame frame = new Frame();
                frame.Decode(buffer);
            });
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecBinaryCountBoundsCheckTest()
        {
            byte[] payload = new byte[] { 0xb0, 0x7f, 0xff, 0xff, 0xff };
            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeBinary(buffer));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);

            byte[] payload8 = new byte[] { 0xa0, 0xff };
            buffer = new ByteBuffer(new ArraySegment<byte>(payload8));
            ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeBinary(buffer));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecArrayIntegerOverflowTest()
        {
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                0x7F, 0xFF, 0xFF, 0xFF,
                FormatCode.ULong
            };

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                EncodingBase encoding = AmqpEncoding.GetEncoding(FormatCode.Array32);
                AmqpException ex = Assert.Throws<AmqpException>(() => encoding.DecodeObject(buffer, 0));
                Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeArray<ulong>(buffer));
                Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }
        }

        [Fact]
        public void AmqpCodecZeroWidthCountExceedsLimitTest()
        {
            // Single array of UInt0 unbounded elements exceeding MaxUnboundedSize.
            // count=20000 × FixedWidth.UInt (4) = 80000 bytes > MaxUnboundedSize (65536).
            int count = 20000;
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,     // size = 5 (count field + constructor)
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.UInt0,           // constructor = uint0
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            EncodingBase encoding = AmqpEncoding.GetEncoding(FormatCode.Array32);
            AmqpException ex = Assert.Throws<AmqpException>(() => encoding.DecodeObject(buffer, 0));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecNestedArrayDepthTest()
        {
            // Nested arrays exceeding MaxNestingDepth (64).
            // Build 70 nesting levels of Array32, each containing 1 element
            // of the next level. Innermost is a single uint0.
            int levels = 70;

            // Build from inside out. Innermost: Array32 with 1 uint0
            byte[] inner = new byte[]
            {
                0x00, 0x00, 0x00, 0x05,     // size = 5
                0x00, 0x00, 0x00, 0x01,     // count = 1
                FormatCode.UInt0,           // constructor = uint0
            };

            for (int i = 1; i < levels; i++)
            {
                int contentSize = inner.Length;
                int size = 4 + 1 + contentSize; // count(4) + constructor(1) + inner
                byte[] outer = new byte[8 + 1 + contentSize]; // size(4) + count(4) + constructor(1) + inner
                outer[0] = (byte)(size >> 24);
                outer[1] = (byte)(size >> 16);
                outer[2] = (byte)(size >> 8);
                outer[3] = (byte)size;
                outer[4] = 0; outer[5] = 0; outer[6] = 0; outer[7] = 1; // count = 1
                outer[8] = FormatCode.Array32; // constructor
                Buffer.BlockCopy(inner, 0, outer, 9, contentSize);
                inner = outer;
            }

            // Prepend the outermost Array32 format code
            byte[] payload = new byte[1 + inner.Length];
            payload[0] = FormatCode.Array32;
            Buffer.BlockCopy(inner, 0, payload, 1, inner.Length);

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            EncodingBase encoding = AmqpEncoding.GetEncoding(FormatCode.Array32);
            AmqpException ex = Assert.Throws<AmqpException>(() => encoding.DecodeObject(buffer, 0));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedArrayValidTest()
        {
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x17,
                0x00, 0x00, 0x00, 0x02,
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                0x00, 0x00, 0x00, 0x03,
                FormatCode.UInt0,
                0x00, 0x00, 0x00, 0x05,
                0x00, 0x00, 0x00, 0x02,
                FormatCode.UInt0,
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            EncodingBase encoding = AmqpEncoding.GetEncoding(FormatCode.Array32);
            object result = encoding.DecodeObject(buffer, 0);

            Assert.NotNull(result);
            Array[] arrays = (Array[])result;
            Assert.Equal(2, arrays.Length);
            Assert.Equal(3, arrays[0].Length);
            Assert.Equal(2, arrays[1].Length);

            buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            uint[][] staticResult = AmqpCodec.DecodeArray<uint[]>(buffer);

            Assert.NotNull(staticResult);
            Assert.Equal(2, staticResult.Length);
            Assert.Equal(3, staticResult[0].Length);
            Assert.Equal(2, staticResult[1].Length);
        }

        [Fact]
        public void AmqpCodecArrayCountLimitStaticDecodeTest()
        {
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                0x7F, 0xFF, 0xFF, 0xFF,
                FormatCode.UInt,
            };

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeArray<uint>(buffer));
                Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                EncodingBase encoding = AmqpEncoding.GetEncoding(FormatCode.Array32);
                AmqpException ex = Assert.Throws<AmqpException>(() => encoding.DecodeObject(buffer, FormatCode.Array32));
                Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }
        }

        [Fact]
        public void AmqpCodecNestedDescribedTypeDepthTest()
        {
            int depth = 10000;
            byte[] payload = new byte[(depth * 3) + 1];
            int offset = 0;
            for (int i = 0; i < depth; i++)
            {
                payload[offset++] = FormatCode.Described;
                payload[offset++] = FormatCode.SmallULong;
                payload[offset++] = 0x01;
            }
            payload[offset] = FormatCode.Null;

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedDescribedDescriptorDepthTest()
        {
            int depth = 10000;
            byte[] payload = new byte[depth + 2];
            for (int i = 0; i < depth; i++)
            {
                payload[i] = FormatCode.Described;
            }
            payload[depth] = FormatCode.Null;
            payload[depth + 1] = FormatCode.Null;

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedListDepthTest()
        {
            int depth = 150;
            byte[] inner = new byte[] { FormatCode.Null };
            for (int i = 0; i < depth; i++)
            {
                int contentSize = inner.Length;
                byte[] outer = new byte[contentSize + 9];
                outer[0] = FormatCode.List32;
                int size = 4 + contentSize;
                outer[1] = (byte)(size >> 24);
                outer[2] = (byte)(size >> 16);
                outer[3] = (byte)(size >> 8);
                outer[4] = (byte)size;
                outer[5] = 0; outer[6] = 0; outer[7] = 0; outer[8] = 1;
                Buffer.BlockCopy(inner, 0, outer, 9, contentSize);
                inner = outer;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedDescribedListDepthTest()
        {
            int depth = 100;
            byte[] inner = new byte[] { FormatCode.Null };
            for (int i = 0; i < depth; i++)
            {
                int contentSize = inner.Length;
                byte[] listed = new byte[contentSize + 9];
                listed[0] = FormatCode.List32;
                int size = 4 + contentSize;
                listed[1] = (byte)(size >> 24);
                listed[2] = (byte)(size >> 16);
                listed[3] = (byte)(size >> 8);
                listed[4] = (byte)size;
                listed[5] = 0; listed[6] = 0; listed[7] = 0; listed[8] = 1;
                Buffer.BlockCopy(inner, 0, listed, 9, contentSize);

                byte[] described = new byte[listed.Length + 3];
                described[0] = FormatCode.Described;
                described[1] = FormatCode.SmallULong;
                described[2] = 0x01;
                Buffer.BlockCopy(listed, 0, described, 3, listed.Length);

                inner = described;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedMapDepthTest()
        {
            int depth = 150;
            byte[] inner = new byte[] { FormatCode.Null };
            for (int i = 0; i < depth; i++)
            {
                int contentSize = 1 + inner.Length;
                byte[] outer = new byte[contentSize + 9];
                outer[0] = FormatCode.Map32;
                int size = 4 + contentSize;
                outer[1] = (byte)(size >> 24);
                outer[2] = (byte)(size >> 16);
                outer[3] = (byte)(size >> 8);
                outer[4] = (byte)size;
                outer[5] = 0; outer[6] = 0; outer[7] = 0; outer[8] = 2;
                outer[9] = FormatCode.Null;
                Buffer.BlockCopy(inner, 0, outer, 10, inner.Length);
                inner = outer;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedDescribedMapDepthTest()
        {
            int depth = 100;
            byte[] inner = new byte[] { FormatCode.Null };
            for (int i = 0; i < depth; i++)
            {
                int contentSize = 1 + inner.Length;
                byte[] mapped = new byte[contentSize + 9];
                mapped[0] = FormatCode.Map32;
                int size = 4 + contentSize;
                mapped[1] = (byte)(size >> 24);
                mapped[2] = (byte)(size >> 16);
                mapped[3] = (byte)(size >> 8);
                mapped[4] = (byte)size;
                mapped[5] = 0; mapped[6] = 0; mapped[7] = 0; mapped[8] = 2;
                mapped[9] = FormatCode.Null;
                Buffer.BlockCopy(inner, 0, mapped, 10, inner.Length);

                byte[] described = new byte[mapped.Length + 3];
                described[0] = FormatCode.Described;
                described[1] = FormatCode.SmallULong;
                described[2] = 0x01;
                Buffer.BlockCopy(mapped, 0, described, 3, mapped.Length);

                inner = described;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedMapCompoundKeyDepthTest()
        {
            int nestingDepth = 200;
            byte[] key = new byte[nestingDepth + 2];
            for (int i = 0; i < nestingDepth; i++)
            {
                key[i] = FormatCode.Described;
            }
            key[nestingDepth] = FormatCode.Null;
            key[nestingDepth + 1] = FormatCode.Null;

            int contentSize = key.Length + 1;
            byte[] payload = new byte[contentSize + 9];
            payload[0] = FormatCode.Map32;
            int size = 4 + contentSize;
            payload[1] = (byte)(size >> 24);
            payload[2] = (byte)(size >> 16);
            payload[3] = (byte)(size >> 8);
            payload[4] = (byte)size;
            payload[5] = 0; payload[6] = 0; payload[7] = 0; payload[8] = 2;
            Buffer.BlockCopy(key, 0, payload, 9, key.Length);
            payload[payload.Length - 1] = FormatCode.Null;

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedDescribedListMapRotationDepthTest()
        {
            int rotations = 50;
            byte[] inner = new byte[] { FormatCode.Null };
            for (int i = 0; i < rotations; i++)
            {
                int mapContentSize = 1 + inner.Length;
                byte[] mapped = new byte[mapContentSize + 9];
                mapped[0] = FormatCode.Map32;
                int mapSize = 4 + mapContentSize;
                mapped[1] = (byte)(mapSize >> 24);
                mapped[2] = (byte)(mapSize >> 16);
                mapped[3] = (byte)(mapSize >> 8);
                mapped[4] = (byte)mapSize;
                mapped[5] = 0; mapped[6] = 0; mapped[7] = 0; mapped[8] = 2;
                mapped[9] = FormatCode.Null;
                Buffer.BlockCopy(inner, 0, mapped, 10, inner.Length);

                int listContentSize = mapped.Length;
                byte[] listed = new byte[listContentSize + 9];
                listed[0] = FormatCode.List32;
                int listSize = 4 + listContentSize;
                listed[1] = (byte)(listSize >> 24);
                listed[2] = (byte)(listSize >> 16);
                listed[3] = (byte)(listSize >> 8);
                listed[4] = (byte)listSize;
                listed[5] = 0; listed[6] = 0; listed[7] = 0; listed[8] = 1;
                Buffer.BlockCopy(mapped, 0, listed, 9, mapped.Length);

                byte[] described = new byte[listed.Length + 3];
                described[0] = FormatCode.Described;
                described[1] = FormatCode.SmallULong;
                described[2] = 0x01;
                Buffer.BlockCopy(listed, 0, described, 3, listed.Length);

                inner = described;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecNestedListMapArrayRotationDepthTest()
        {
            // Mixed compound nesting: list(map(null => array[list(map(null => array[...]))]))
            // Each rotation = map(+1) + array(+1) + list(+1) = 3 depth increments.
            // 25 rotations + 1 outer list = 76 depth > MaxNestingDepth (64).
            int rotations = 25;

            // Innermost: a single UInt value
            byte[] inner = new byte[] { FormatCode.UInt, 0x00, 0x00, 0x00, 0x01 };

            for (int i = 0; i < rotations; i++)
            {
                // 1. Wrap inner in List32(count=1)
                int listContentSize = inner.Length;
                int listSize = 4 + listContentSize;
                byte[] listed = new byte[9 + listContentSize];
                listed[0] = FormatCode.List32;
                listed[1] = (byte)(listSize >> 24);
                listed[2] = (byte)(listSize >> 16);
                listed[3] = (byte)(listSize >> 8);
                listed[4] = (byte)listSize;
                listed[5] = 0; listed[6] = 0; listed[7] = 0; listed[8] = 1;
                Buffer.BlockCopy(inner, 0, listed, 9, inner.Length);

                // 2. Wrap in Array32(count=1, constructor=List32)
                int elemBodyLen = listed.Length - 1;
                int arraySize = 4 + 1 + elemBodyLen;
                byte[] arrayed = new byte[10 + elemBodyLen];
                arrayed[0] = FormatCode.Array32;
                arrayed[1] = (byte)(arraySize >> 24);
                arrayed[2] = (byte)(arraySize >> 16);
                arrayed[3] = (byte)(arraySize >> 8);
                arrayed[4] = (byte)arraySize;
                arrayed[5] = 0; arrayed[6] = 0; arrayed[7] = 0; arrayed[8] = 1;
                arrayed[9] = FormatCode.List32;
                Buffer.BlockCopy(listed, 1, arrayed, 10, elemBodyLen);

                // 3. Wrap in Map32(count=2: null key + array value)
                int mapContentSize = 1 + arrayed.Length;
                int mapSize = 4 + mapContentSize;
                byte[] mapped = new byte[9 + mapContentSize];
                mapped[0] = FormatCode.Map32;
                mapped[1] = (byte)(mapSize >> 24);
                mapped[2] = (byte)(mapSize >> 16);
                mapped[3] = (byte)(mapSize >> 8);
                mapped[4] = (byte)mapSize;
                mapped[5] = 0; mapped[6] = 0; mapped[7] = 0; mapped[8] = 2;
                mapped[9] = FormatCode.Null;
                Buffer.BlockCopy(arrayed, 0, mapped, 10, arrayed.Length);

                inner = mapped;
            }

            // Final wrap in List32
            int outerContentSize = inner.Length;
            int outerSize = 4 + outerContentSize;
            byte[] payload = new byte[9 + outerContentSize];
            payload[0] = FormatCode.List32;
            payload[1] = (byte)(outerSize >> 24);
            payload[2] = (byte)(outerSize >> 16);
            payload[3] = (byte)(outerSize >> 8);
            payload[4] = (byte)outerSize;
            payload[5] = 0; payload[6] = 0; payload[7] = 0; payload[8] = 1;
            Buffer.BlockCopy(inner, 0, payload, 9, inner.Length);

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecArrayDynamicResizingTest()
        {
            // Array with count=500, constructor=UInt, 500 x 4-byte uint values.
            // Dynamic resizing should handle this without pre-allocating count elements.
            int itemCount = 500;
            int itemSize = 4;
            int contentSize = itemCount * itemSize;
            int size = 4 + 1 + contentSize;
            byte[] payload = new byte[1 + 4 + 4 + 1 + contentSize];

            int offset = 0;
            payload[offset++] = FormatCode.Array32;
            payload[offset++] = (byte)(size >> 24);
            payload[offset++] = (byte)(size >> 16);
            payload[offset++] = (byte)(size >> 8);
            payload[offset++] = (byte)size;
            payload[offset++] = (byte)(itemCount >> 24);
            payload[offset++] = (byte)(itemCount >> 16);
            payload[offset++] = (byte)(itemCount >> 8);
            payload[offset++] = (byte)itemCount;
            payload[offset++] = FormatCode.UInt;
            for (int i = 0; i < itemCount; i++)
            {
                payload[offset++] = 0;
                payload[offset++] = 0;
                payload[offset++] = (byte)(i >> 8);
                payload[offset++] = (byte)i;
            }

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            uint[] result = AmqpCodec.DecodeArray<uint>(buffer);

            Assert.Equal(itemCount, result.Length);
            for (int i = 0; i < itemCount; i++)
            {
                Assert.Equal((uint)i, result[i]);
            }
        }

        [Fact]
        public void AmqpCodecZeroWidthBooleanTrackingTest()
        {
            // Array of BooleanTrue items exceeding MaxUnboundedSize.
            int count = 65536 / 1 + 1;
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.BooleanTrue,
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecZeroWidthULongTrackingTest()
        {
            // Array of ULong0 items exceeding MaxUnboundedSize.
            int count = 65536 / 8 + 1;
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.ULong0,
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecZeroWidthList0TrackingTest()
        {
            // Array of List0 items — List0 is unbounded but lists are reference types.
            // Buffer is too small to contain 12000 items, so it fails from buffer exhaustion.
            int count = 12000;
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.List0,
            };

            // Expected: buffer underflow (AmqpException or ArgumentOutOfRange)
            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            Assert.ThrowsAny<Exception>(() => AmqpCodec.DecodeObject(buffer));
        }

        [Fact]
        public void AmqpCodecZeroWidthUnderLimitTest()
        {
            // Array of UInt0 items at exactly MaxUnboundedSize — should succeed.
            int count = 65536 / 4; // 16384 × 4 = 65536 = limit
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.UInt0,
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            object result = AmqpCodec.DecodeObject(buffer);

            Assert.NotNull(result);
            uint[] arr = (uint[])result;
            Assert.Equal(count, arr.Length);
        }

        [Fact]
        public void AmqpCodecMapKeyZeroWidthArrayTrackingTest()
        {
            // Map with many keys that are ULong0 arrays. The cumulative unbounded size
            // across all map keys should exceed MaxUnboundedSize.
            int itemsPerKey = 65536 / 8 / 4; // ~2048
            int numKeys = 8; // 8 keys × 2048 × 8 = 131072 > MaxUnboundedSize (65536)

            byte[] arrayKey = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(itemsPerKey >> 24), (byte)(itemsPerKey >> 16), (byte)(itemsPerKey >> 8), (byte)itemsPerKey,
                FormatCode.ULong0,
            };

            int entrySize = arrayKey.Length + 1; // +1 for FormatCode.Null
            int mapCount = numKeys * 2;
            int mapContentSize = numKeys * entrySize;
            int mapSize = 4 + mapContentSize;

            byte[] payload = new byte[1 + 4 + 4 + mapContentSize];
            int offset = 0;
            payload[offset++] = FormatCode.Map32;
            payload[offset++] = (byte)(mapSize >> 24);
            payload[offset++] = (byte)(mapSize >> 16);
            payload[offset++] = (byte)(mapSize >> 8);
            payload[offset++] = (byte)mapSize;
            payload[offset++] = (byte)(mapCount >> 24);
            payload[offset++] = (byte)(mapCount >> 16);
            payload[offset++] = (byte)(mapCount >> 8);
            payload[offset++] = (byte)mapCount;

            for (int i = 0; i < numKeys; i++)
            {
                Buffer.BlockCopy(arrayKey, 0, payload, offset, arrayKey.Length);
                offset += arrayKey.Length;
                payload[offset++] = FormatCode.Null;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecDecodeObjectFormatCodeOverloadTest()
        {
            // Regression test: DecodeObject(buffer, formatCode) must dispatch via FormatCode
            // overload, NOT interpret formatCode as depth.
            ByteBuffer bf = new ByteBuffer(256, true);
            AmqpCodec.EncodeList(new List<object> { "hello", 42 }, bf);

            FormatCode formatCode = AmqpEncoding.ReadFormatCode(bf);
            object result = AmqpEncoding.DecodeObject(bf, formatCode);

            Assert.NotNull(result);
            List<object> list = result as List<object>;
            Assert.NotNull(list);
            Assert.Equal(2, list.Count);
            Assert.Equal("hello", list[0]);
            Assert.Equal(42, list[1]);
        }

        [Fact]
        public void AmqpCodecListSiblingDepthTest()
        {
            // Flat list with 100 items should not accumulate depth across siblings.
            int itemCount = 100;
            ByteBuffer bf = new ByteBuffer(4096, true);
            var list = new List<object>();
            for (int i = 0; i < itemCount; i++)
            {
                list.Add(i);
            }
            AmqpCodec.EncodeList(list, bf);

            object result = AmqpCodec.DecodeObject(bf);
            List<object> decoded = result as List<object>;
            Assert.NotNull(decoded);
            Assert.Equal(itemCount, decoded.Count);
            for (int i = 0; i < itemCount; i++)
            {
                Assert.Equal(i, decoded[i]);
            }
        }

        [Fact]
        public void AmqpCodecMapSiblingDepthTest()
        {
            // Flat map with 100 entries should not accumulate depth across siblings.
            int entryCount = 100;
            ByteBuffer bf = new ByteBuffer(8192, true);
            AmqpMap map = new AmqpMap();
            for (int i = 0; i < entryCount; i++)
            {
                map.Add(new MapKey("key" + i), i);
            }
            AmqpCodec.EncodeMap(map, bf);

            object result = AmqpCodec.DecodeObject(bf);
            AmqpMap decoded = result as AmqpMap;
            Assert.NotNull(decoded);
            Assert.Equal(entryCount, decoded.Count);
        }

        [Fact]
        public void AmqpCodecLargeListNoCountCapTest()
        {
            // Lists exceeding the old MaxAmqpCollectionCount (65536) should decode correctly.
            int itemCount = 70000;
            ByteBuffer bf = new ByteBuffer(itemCount * 2 + 64, true);
            var list = new List<object>();
            for (int i = 0; i < itemCount; i++)
            {
                list.Add((byte)(i % 256));
            }
            AmqpCodec.EncodeList(list, bf);

            object result = AmqpCodec.DecodeObject(bf);
            List<object> decoded = result as List<object>;
            Assert.NotNull(decoded);
            Assert.Equal(itemCount, decoded.Count);
        }

        [Fact]
        public void AmqpCodecDescribedTypeSiblingDepthTest()
        {
            // A list of described types should not accumulate depth across siblings.
            int count = 50;
            ByteBuffer bf = new ByteBuffer(4096, true);
            var list = new List<object>();
            for (int i = 0; i < count; i++)
            {
                list.Add(new DescribedType((ulong)i, "value" + i));
            }
            AmqpCodec.EncodeList(list, bf);

            object result = AmqpCodec.DecodeObject(bf);
            List<object> decoded = result as List<object>;
            Assert.NotNull(decoded);
            Assert.Equal(count, decoded.Count);
            for (int i = 0; i < count; i++)
            {
                DescribedType dt = decoded[i] as DescribedType;
                Assert.NotNull(dt);
                Assert.Equal((ulong)i, dt.Descriptor);
                Assert.Equal("value" + i, dt.Value);
            }
        }

        [Fact]
        public void AmqpCodecNestedArraysAccumulateUnboundedTest()
        {
            // Multiple arrays of unbounded elements inside a list share one counter.
            // 3 arrays × 6000 UInt0 items × 4 bytes/item = 72000 > 65536.
            int perArrayCount = 6000;
            int numArrays = 3;

            byte[] innerArray = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(perArrayCount >> 24), (byte)(perArrayCount >> 16), (byte)(perArrayCount >> 8), (byte)perArrayCount,
                FormatCode.UInt0,
            };

            int listItemsSize = numArrays * innerArray.Length;
            int listSize = 4 + listItemsSize;
            byte[] payload = new byte[1 + 4 + 4 + listItemsSize];
            int offset = 0;
            payload[offset++] = FormatCode.List32;
            payload[offset++] = (byte)(listSize >> 24);
            payload[offset++] = (byte)(listSize >> 16);
            payload[offset++] = (byte)(listSize >> 8);
            payload[offset++] = (byte)listSize;
            payload[offset++] = (byte)(numArrays >> 24);
            payload[offset++] = (byte)(numArrays >> 16);
            payload[offset++] = (byte)(numArrays >> 8);
            payload[offset++] = (byte)numArrays;
            for (int a = 0; a < numArrays; a++)
            {
                Buffer.BlockCopy(innerArray, 0, payload, offset, innerArray.Length);
                offset += innerArray.Length;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecMapContainingUnboundedArrayTest()
        {
            // Map value is an array of UInt0 exceeding MaxUnboundedSize.
            int count = (65536 / 4) + 1;

            byte[] key = new byte[] { FormatCode.SmallULong, 0x01 };
            byte[] value = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.UInt0,
            };

            int mapItemsSize = key.Length + value.Length;
            int mapSize = 4 + mapItemsSize;
            int mapCount = 2;
            byte[] payload = new byte[1 + 4 + 4 + mapItemsSize];
            int offset = 0;
            payload[offset++] = FormatCode.Map32;
            payload[offset++] = (byte)(mapSize >> 24);
            payload[offset++] = (byte)(mapSize >> 16);
            payload[offset++] = (byte)(mapSize >> 8);
            payload[offset++] = (byte)mapSize;
            payload[offset++] = (byte)(mapCount >> 24);
            payload[offset++] = (byte)(mapCount >> 16);
            payload[offset++] = (byte)(mapCount >> 8);
            payload[offset++] = (byte)mapCount;
            Buffer.BlockCopy(key, 0, payload, offset, key.Length);
            offset += key.Length;
            Buffer.BlockCopy(value, 0, payload, offset, value.Length);

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecDescribedContainingUnboundedArrayTest()
        {
            // Described type containing an array of UInt0 exceeding MaxUnboundedSize.
            int count = (65536 / 4) + 1;

            byte[] payload = new byte[]
            {
                FormatCode.Described,
                FormatCode.SmallULong, 0x09,
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.UInt0,
            };

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecBooleanFalseUnboundedTest()
        {
            // BooleanFalse (0x42) is also unbounded — verify it's tracked.
            int count = (65536 / 1) + 1;
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x05,
                (byte)(count >> 24), (byte)(count >> 16), (byte)(count >> 8), (byte)count,
                FormatCode.BooleanFalse,
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.Contains("unbounded", ex.Error.Description);
        }

        [Fact]
        public void AmqpCodecDynamicResizeLargeNormalArrayTest()
        {
            // Dynamic resizing for normal arrays with count > MaxInitialSize (1024).
            int count = 2000;
            ByteBuffer bf = new ByteBuffer(count * 8, true);
            int[] original = new int[count];
            for (int i = 0; i < count; i++)
            {
                original[i] = i * 7 + 3;
            }

            AmqpCodec.EncodeObject(original, bf);
            int[] decoded = (int[])AmqpCodec.DecodeObject(bf);

            Assert.Equal(count, decoded.Length);
            for (int i = 0; i < count; i++)
            {
                Assert.Equal(original[i], decoded[i]);
            }
        }

        [Fact]
        public void AmqpCodecNegativeCountRejectedTest()
        {
            // Craft a payload with count = -1 (0xFFFFFFFF as int32), verify rejection.
            byte[] payload = new byte[]
            {
                FormatCode.Array32,
                0x00, 0x00, 0x00, 0x08,
                0xFF, 0xFF, 0xFF, 0xFF,
                FormatCode.UInt,
                0x00, 0x00, 0x00, 0x01,
            };

            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            AmqpException ex = Assert.Throws<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.Equal((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [Fact]
        public void AmqpCodecArrayOfArraysUnboundedAccumulationTest()
        {
            // Array of arrays of UInt0. Each inner array contributes to shared unbounded counter.
            // 5 inner arrays × 4000 UInt0 × 4 = 80000 > 65536.
            int innerCount = 4000;
            int numInner = 5;

            int innerBodySize = 9; // size(4) + count(4) + constructor(1)
            int innerSize = 5; // count(4) + constructor(1)

            int outerSize = 4 + 1 + (numInner * innerBodySize);

            byte[] payload = new byte[1 + 4 + 4 + 1 + (numInner * innerBodySize)];
            int offset = 0;
            payload[offset++] = FormatCode.Array32;
            payload[offset++] = (byte)(outerSize >> 24);
            payload[offset++] = (byte)(outerSize >> 16);
            payload[offset++] = (byte)(outerSize >> 8);
            payload[offset++] = (byte)outerSize;
            payload[offset++] = (byte)(numInner >> 24);
            payload[offset++] = (byte)(numInner >> 16);
            payload[offset++] = (byte)(numInner >> 8);
            payload[offset++] = (byte)numInner;
            payload[offset++] = FormatCode.Array32; // shared constructor

            for (int i = 0; i < numInner; i++)
            {
                payload[offset++] = (byte)(innerSize >> 24);
                payload[offset++] = (byte)(innerSize >> 16);
                payload[offset++] = (byte)(innerSize >> 8);
                payload[offset++] = (byte)innerSize;
                payload[offset++] = (byte)(innerCount >> 24);
                payload[offset++] = (byte)(innerCount >> 16);
                payload[offset++] = (byte)(innerCount >> 8);
                payload[offset++] = (byte)innerCount;
                payload[offset++] = FormatCode.UInt0;
            }

            AmqpException ex = Assert.Throws<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.Contains("unbounded", ex.Error.Description);
        }
    }
}
