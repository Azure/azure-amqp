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
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
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

        [TestMethod]
        public void AmqpCodecSingleValueTest()
        {
            byte[] workBuffer = new byte[2048];
            ByteBuffer buffer;

            // boolean true
            AmqpCodec.EncodeBoolean(boolTrue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(boolTrueBin, 0, boolTrueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            bool? bv = AmqpCodec.DecodeBoolean(new ByteBuffer(new ArraySegment<byte>(boolTrueBin)));
            Assert.IsTrue(bv.Value, "Boolean value is not true.");

            // boolean false
            AmqpCodec.EncodeBoolean(boolFalse, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(boolFalseBin, 0, boolFalseBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            bv = AmqpCodec.DecodeBoolean(new ByteBuffer(new ArraySegment<byte>(boolFalseBin)));
            Assert.IsFalse(bv.Value, "Boolean value is not false.");

            // ubyte
            AmqpCodec.EncodeUByte(ubyteValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ubyteValueBin, 0, ubyteValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            byte? bytev = AmqpCodec.DecodeUByte(new ByteBuffer(new ArraySegment<byte>(ubyteValueBin)));
            Assert.IsTrue(bytev == ubyteValue, "UByte value is not equal.");

            // ushort
            AmqpCodec.EncodeUShort(ushortValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ushortValueBin, 0, ushortValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ushort? ushortv = AmqpCodec.DecodeUShort(new ByteBuffer(new ArraySegment<byte>(ushortValueBin)));
            Assert.IsTrue(ushortv == ushortValue, "UShort value is not equal.");

            // uint0
            AmqpCodec.EncodeUInt(uint0Value, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uint0ValueBin, 0, uint0ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            uint? uint0v = AmqpCodec.DecodeUInt(new ByteBuffer(new ArraySegment<byte>(uint0ValueBin)));
            Assert.IsTrue(uint0v == uint0Value, "UInt0 value is not equal.");

            // uint small
            AmqpCodec.EncodeUInt(uintSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uintSmallValueBin, 0, uintSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            uint? uintSmallV = AmqpCodec.DecodeUInt(new ByteBuffer(new ArraySegment<byte>(uintSmallValueBin)));
            Assert.IsTrue(uintSmallV == uintSmallValue, "UIntSmall value is not equal.");

            // uint
            AmqpCodec.EncodeUInt(uintValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uintValueBin, 0, uintValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            uint? uintv = AmqpCodec.DecodeUInt(new ByteBuffer(new ArraySegment<byte>(uintValueBin)));
            Assert.IsTrue(uintv == uintValue, "UInt value is not equal.");

            // ulong0
            AmqpCodec.EncodeULong(ulong0Value, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ulong0ValueBin, 0, ulong0ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ulong? ulong0v = AmqpCodec.DecodeULong(new ByteBuffer(new ArraySegment<byte>(ulong0ValueBin)));
            Assert.IsTrue(ulong0v == ulong0Value, "ULong0 value is not equal.");

            // ulong small
            AmqpCodec.EncodeULong(ulongSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ulongSmallValueBin, 0, ulongSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ulong? ulongSmallV = AmqpCodec.DecodeULong(new ByteBuffer(new ArraySegment<byte>(ulongSmallValueBin)));
            Assert.IsTrue(ulongSmallV == ulongSmallValue, "ULong value is not equal.");

            // ulong
            AmqpCodec.EncodeULong(ulongValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(ulongValueBin, 0, ulongValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            ulong? ulongv = AmqpCodec.DecodeULong(new ByteBuffer(new ArraySegment<byte>(ulongValueBin)));
            Assert.IsTrue(ulongv == ulongValue, "ULong value is not equal.");

            // byte
            AmqpCodec.EncodeByte(byteValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(byteValueBin, 0, byteValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            sbyte? sbytev = AmqpCodec.DecodeByte(new ByteBuffer(new ArraySegment<byte>(byteValueBin)));
            Assert.IsTrue(sbytev == byteValue, "Byte value is not equal.");

            // short
            AmqpCodec.EncodeShort(shortValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(shortValueBin, 0, shortValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            short? shortv = AmqpCodec.DecodeShort(new ByteBuffer(new ArraySegment<byte>(shortValueBin)));
            Assert.IsTrue(shortv == shortValue, "Short value is not equal.");

            // int small
            AmqpCodec.EncodeInt(intSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(intSmallValueBin, 0, intSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            int? intSmallV = AmqpCodec.DecodeInt(new ByteBuffer(new ArraySegment<byte>(intSmallValueBin)));
            Assert.IsTrue(intSmallV == intSmallValue, "Int small value is not equal.");

            // int
            AmqpCodec.EncodeInt(intValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(intValueBin, 0, intValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            int? intv = AmqpCodec.DecodeInt(new ByteBuffer(new ArraySegment<byte>(intValueBin)));
            Assert.IsTrue(intv == intValue, "Int value is not equal.");

            // long
            AmqpCodec.EncodeLong(longSmallValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(longSmallValueBin, 0, longSmallValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            long? longSmallV = AmqpCodec.DecodeLong(new ByteBuffer(new ArraySegment<byte>(longSmallValueBin)));
            Assert.IsTrue(longSmallV == longSmallValue, "Long small value is not equal.");

            // long
            AmqpCodec.EncodeLong(longValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(longValueBin, 0, longValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            long? longv = AmqpCodec.DecodeLong(new ByteBuffer(new ArraySegment<byte>(longValueBin)));
            Assert.IsTrue(longv == longValue, "Long value is not equal.");

            // float
            AmqpCodec.EncodeFloat(floatValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(floatValueBin, 0, floatValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            float? floatv = AmqpCodec.DecodeFloat(new ByteBuffer(new ArraySegment<byte>(floatValueBin)));
            Assert.IsTrue(floatv == floatValue, "Float value is not equal.");

            // double
            AmqpCodec.EncodeDouble(doubleValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(doubleValueBin, 0, doubleValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            double? doublev = AmqpCodec.DecodeDouble(new ByteBuffer(new ArraySegment<byte>(doubleValueBin)));
            Assert.IsTrue(doublev == doubleValue, "Double value is not equal.");

            //decimal
            decimal? dec32 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal32ValueBin)));
            Assert.IsTrue(dec32.Value == decimal32Value, "Decimal32 value is not equal");

            decimal? dec64 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal64ValueBin)));
            Assert.IsTrue(dec64.Value == decimal64Value, "Decimal64 value is not equal");

            decimal? dec128 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal128ValueBin)));
            Assert.IsTrue(dec128.Value == decimal128Value, "Decimal128 value is not equal");

            // char
            AmqpCodec.EncodeChar(charValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(charValueBin, 0, charValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            char? charv = AmqpCodec.DecodeChar(new ByteBuffer(new ArraySegment<byte>(charValueBin)));
            Assert.IsTrue(charv == charValue, "Char value is not equal.");

            // timestamp
            AmqpCodec.EncodeTimeStamp(dtValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(dtValueBin, 0, dtValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            DateTime? dtv = AmqpCodec.DecodeTimeStamp(new ByteBuffer(new ArraySegment<byte>(dtValueBin)));
            Assert.IsTrue(dtv == dtValue.ToUniversalTime(), "UByte value is not equal.");

            // uuid
            AmqpCodec.EncodeUuid(uuidValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(uuidValueBin, 0, uuidValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            Guid? uuidv = AmqpCodec.DecodeUuid(new ByteBuffer(new ArraySegment<byte>(uuidValueBin)));
            Assert.IsTrue(uuidv == uuidValue, "Uuid value is not equal.");

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
            Assert.IsTrue(symbol8v.Value == strValue, "Symbol8 string value is not equal.");

            // symbol 32
            AmqpSymbol symbol32v = AmqpCodec.DecodeSymbol(new ByteBuffer(new ArraySegment<byte>(sym32ValueBin)));
            Assert.IsTrue(symbol32v.Value == strValue, "Symbol32 string value is not equal.");

            // string 8 UTF8
            AmqpCodec.EncodeString(strValue, buffer = new ByteBuffer(workBuffer));
            EnsureEqual(str8Utf8ValueBin, 0, str8Utf8ValueBin.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            string str8Utf8 = AmqpCodec.DecodeString(new ByteBuffer(new ArraySegment<byte>(str8Utf8ValueBin)));
            Assert.IsTrue(str8Utf8 == strValue, "UTF8 string8 string value is not equal.");

            // string 32 UTF8
            string str32Utf8 = AmqpCodec.DecodeString(new ByteBuffer(new ArraySegment<byte>(str32Utf8ValueBin)));
            Assert.IsTrue(str32Utf8 == strValue, "UTF8 string32 string value is not equal.");
        }

        [TestMethod]
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
            Assert.AreEqual(buffer.Length - 5, listSize);

            IList decList = AmqpCodec.DecodeList(buffer);
            int index = 0;

            Assert.IsTrue(decList[index++].Equals(true), "Boolean true expected.");
            Assert.IsTrue(decList[index++].Equals(false), "Boolean false expected.");
            Assert.IsTrue(decList[index++].Equals(ubyteValue), "UByte value not equal.");
            Assert.IsTrue(decList[index++].Equals(ushortValue), "UShort value not equal.");
            Assert.IsTrue(decList[index++].Equals(uintValue), "UInt value not equal.");
            Assert.IsTrue(decList[index++].Equals(ulongValue), "ULong value not equal.");
            Assert.IsTrue(decList[index++].Equals(byteValue), "Byte value not equal.");
            Assert.IsTrue(decList[index++].Equals(shortValue), "Short value not equal.");
            Assert.IsTrue(decList[index++].Equals(intValue), "Int value not equal.");
            Assert.IsTrue(decList[index++].Equals(longValue), "Long value not equal.");
            Assert.IsTrue(decList[index++] == null, "Null object expected.");
            Assert.IsTrue(decList[index++].Equals(floatValue), "Float value not equal.");
            Assert.IsTrue(decList[index++].Equals(doubleValue), "Double value not equal.");
            Assert.IsTrue(decList[index++].Equals(decimal32Value), "Decimal32 value not equal.");
            Assert.IsTrue(decList[index++].Equals(decimal64Value), "Decimal64 value not equal.");
            Assert.IsTrue(decList[index++].Equals(decimal128Value), "Decimal128 value not equal.");
            Assert.IsTrue(decList[index++].Equals(charValue), "Char value not equal.");
            Assert.IsTrue(decList[index++].Equals(dtValue), "TimeStamp value not equal.");
            Assert.IsTrue(decList[index++].Equals(uuidValue), "Uuid value not equal.");

            Assert.IsTrue(decList[index++] == null, "Null binary expected.");
            ArraySegment<byte> bin8 = (ArraySegment<byte>)decList[index++];
            EnsureEqual(bin8.Array, bin8.Offset, bin8.Count, bin8Value.Array, bin8Value.Offset, bin8Value.Count);
            ArraySegment<byte> bin32 = (ArraySegment<byte>)decList[index++];
            EnsureEqual(bin32.Array, bin32.Offset, bin32.Count, bin32Value.Array, bin32Value.Offset, bin32Value.Count);

            Assert.IsTrue(decList[index++] == null, "Null symbol expected.");
            AmqpSymbol symDecode = (AmqpSymbol)decList[index++];
            Assert.IsTrue(symDecode.Equals(strValue), "AmqpSymbol value not equal.");
            symDecode = (AmqpSymbol)decList[index++];
            Assert.IsTrue(symDecode.Equals(strBig), "AmqpSymbol value (big) not equal.");

            string strDecode = (string)decList[index++];
            Assert.IsTrue(strDecode.Equals(strValue), "string value not equal.");
            strDecode = (string)decList[index++];
            Assert.IsTrue(strDecode.Equals(strBig), "string value (big) not equal.");

            DescribedType described = (DescribedType)decList[index++];
            Assert.IsTrue(described.Descriptor.Equals(described1.Descriptor), "Described value 1 descriptor is different");
            Assert.IsTrue(described.Value.Equals(described1.Value), "Described value 1 value is different");
            described = (DescribedType)decList[index++];
            Assert.IsTrue(described.Descriptor.Equals(described2.Descriptor), "Described value 2 descriptor is different");
            Assert.IsTrue(described.Value.Equals(described2.Value), "Described value 2 value is different");
            described = (DescribedType)decList[index++];
            Assert.IsTrue(described.Descriptor.Equals(described3.Descriptor), "Described value 3 descriptor is different");
            Assert.IsTrue(described.Value.Equals(described3.Value), "Described value 3 value is different");
            described = (DescribedType)decList[index++];
            EnsureEqual((DateTime)described4.Descriptor, (DateTime)described.Descriptor);
            EnsureEqual((IList)described.Value, (IList)described4.Value);
        }

        [TestMethod]
        public void AmqpCodecList0Test()
        {
            byte[] list0Bin = new byte[] { 0x45 };
            byte[] workBuffer = new byte[128];
            ByteBuffer buffer = new ByteBuffer(workBuffer);

            List<object> list0 = new List<object>();
            AmqpCodec.EncodeList(list0, buffer);
            EnsureEqual(list0Bin, 0, list0Bin.Length, buffer.Buffer, buffer.Offset, buffer.Length);

            IList list0v = AmqpCodec.DecodeList(buffer);
            Assert.IsTrue(list0v.Count == 0, "The list should contain 0 items.");
        }

        [TestMethod]
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
            Assert.AreEqual(buffer.Length - 5, mapSize);

            AmqpMap decMap = AmqpCodec.DecodeMap(buffer);

            Assert.IsTrue(decMap[new MapKey("boolTrue")].Equals(true), "Boolean true expected.");
            Assert.IsTrue(decMap[new MapKey("boolFalse")].Equals(false), "Boolean false expected.");
            Assert.IsTrue(decMap[new MapKey("ubyte")].Equals(ubyteValue), "UByte value not equal.");
            Assert.IsTrue(decMap[new MapKey("ushort")].Equals(ushortValue), "UShort value not equal.");
            Assert.IsTrue(decMap[new MapKey("uint")].Equals(uintValue), "UInt value not equal.");
            Assert.IsTrue(decMap[new MapKey("ulong")].Equals(ulongValue), "ULong value not equal.");
            Assert.IsTrue(decMap[new MapKey("byte")].Equals(byteValue), "Byte value not equal.");
            Assert.IsTrue(decMap[new MapKey("short")].Equals(shortValue), "Short value not equal.");
            Assert.IsTrue(decMap[new MapKey("int")].Equals(intValue), "Int value not equal.");
            Assert.IsTrue(decMap[new MapKey("long")].Equals(longValue), "Long value not equal.");
            Assert.IsTrue(decMap[new MapKey("null")] == null, "Null object expected.");
            Assert.IsTrue(decMap[new MapKey("float")].Equals(floatValue), "Float value not equal.");
            Assert.IsTrue(decMap[new MapKey("double")].Equals(doubleValue), "Double value not equal.");
            Assert.IsTrue(decMap[new MapKey("decimal32")].Equals(decimal32Value), "Decimal32 value not equal.");
            Assert.IsTrue(decMap[new MapKey("decimal64")].Equals(decimal64Value), "Decimal64 value not equal.");
            Assert.IsTrue(decMap[new MapKey("decimal128")].Equals(decimal128Value), "Decimal128 value not equal.");
            Assert.IsTrue(decMap[new MapKey("char")].Equals(charValue), "Char value not equal.");
            Assert.IsTrue(decMap[new MapKey("datetime")].Equals(dtValue), "TimeStamp value not equal.");
            Assert.IsTrue(decMap[new MapKey("uuid")].Equals(uuidValue), "Uuid value not equal.");
            Assert.IsTrue(decMap[new MapKey("binaryNull")] == null, "Null binary expected.");
            ArraySegment<byte> bin8 = (ArraySegment<byte>)decMap[new MapKey("binary8")];
            EnsureEqual(bin8.Array, bin8.Offset, bin8.Count, bin8Value.Array, bin8Value.Offset, bin8Value.Count);
            ArraySegment<byte> bin32 = (ArraySegment<byte>)decMap[new MapKey("binary32")];
            EnsureEqual(bin32.Array, bin32.Offset, bin32.Count, bin32Value.Array, bin32Value.Offset, bin32Value.Count);

            Assert.IsTrue(decMap[new MapKey("symbolNull")] == null, "Null symbol expected.");
            AmqpSymbol symDecode = (AmqpSymbol)decMap[new MapKey("symbol8")];
            Assert.IsTrue(symDecode.Equals(strValue), "AmqpSymbol value not equal.");
            symDecode = (AmqpSymbol)decMap[new MapKey("symbol32")];
            Assert.IsTrue(symDecode.Equals(strBig), "AmqpSymbol value (big) not equal.");

            string strDecode = (string)decMap[new MapKey("string8")];
            Assert.IsTrue(strDecode.Equals(strValue), "string value not equal.");
            strDecode = (string)decMap[new MapKey("string32")];
            Assert.IsTrue(strDecode.Equals(strBig), "string value (big) not equal.");

            DescribedType described = (DescribedType)decMap[new MapKey("described1")];
            Assert.IsTrue(described.Descriptor.Equals(described1.Descriptor), "Described value 1 descriptor is different");
            Assert.IsTrue(described.Value.Equals(described1.Value), "Described value 1 value is different");
        }

        [TestMethod]
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

            Assert.IsTrue(nullDecoded == null, "the null multiple value is not null");
            Assert.IsTrue(Multiple<string>.Intersect(oneValue, oneDecoded).Count == 1, "multiple of one string value failed");
            Assert.IsTrue(Multiple<Guid>.Intersect(twoValues, twoDecoded).Count == 2, "multiple of two uuid values failed");
            Assert.IsTrue(Multiple<AmqpSymbol>.Intersect(threeValues, threeDecoded).Count == 3, "multiple of three symbol values failed");
        }

        [TestMethod]
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

            Assert.IsTrue(buffer.Length == 0, "All bytes in the buffer should be consumed");
        }

        [TestMethod]
        public void AmqpCodecDescribedArrayTest()
        {
            int size = AmqpCodec.GetObjectEncodeSize(described5);
            ByteBuffer buffer = new ByteBuffer(new byte[size]);
            AmqpCodec.EncodeObject(described5, buffer);
            DescribedType decoded = (DescribedType)AmqpCodec.DecodeObject(buffer);
            Assert.IsTrue(decoded.Descriptor.Equals(described5.Descriptor), "Descriptor value not equal");
            string[] original = (string[])described5.Value;
            string[] array = (string[])decoded.Value;
            Assert.IsTrue(original.Length == array.Length, string.Format("length not equal {0} != {1}", original.Length, array.Length));
            for (int i = 0; i < original.Length; ++i)
            {
                Assert.IsTrue(original[i] == array[i], string.Format("index {0}: {1} != {2}", i, original[i], array[i]));
            }
        }

        [TestMethod]
        public void AmqpCodecArrayTest()
        {
            ArrayTest<bool>(
                new bool[] { true, false, false, true, false, false, true },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<byte>(
                new byte[] { 0, 1, 2, 3, 4, 200, 255 },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<ushort>(
                new ushort[] { 0, 1, 2, 0x1234, 0xab00, 0xffff },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<uint>(
                new uint[] { 0, 1, 2, 0x1234, 0xab00, 0xffff, 0x239d9e, 0xffffffff },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<ulong>(
                new ulong[] { 0, 1, 2, 0x1234, 0xab00, 0xffff, 0x239d9e, 0xffffffff, 0x329999999, 0xffffffffffffffff },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<sbyte>(
                new sbyte[] { -127, -9, 0, 9, 127 },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<short>(
                new short[] { short.MinValue, -127, -9, 0, 9, 127, short.MaxValue},
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<int>(
                new int[] { int.MinValue, short.MinValue, - 127, -9, 0, 9, 127, short.MaxValue, int.MaxValue },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<long>(
                new long[] { long.MinValue, int.MinValue, short.MinValue, -127, -9, 0, 9, 127, short.MaxValue, int.MaxValue, long.MaxValue },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<float>(
                new float[] { float.MinValue, -238.233453f, 0, 89234.92394f, float.MaxValue },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<double>(
                new double[] { double.MinValue, float.MinValue, -238.233453f, 0, 89234.92394f, float.MaxValue, double.MaxValue },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<decimal>(
                new decimal[] { decimal.MinValue, -234934.092348m, 0, 38743947394.2349324m, decimal.MaxValue },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<char>(
                new char[] { 'a', 'b', 'A', 'C' },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<DateTime>(
                new DateTime[] { DateTime.Now - TimeSpan.FromDays(100), DateTime.Now, DateTime.Now + TimeSpan.FromDays(100) },
                (n1, n2) => { EnsureEqual(n1, n2); });

            ArrayTest<Guid>(
                new Guid[] { Guid.NewGuid(), Guid.NewGuid(), Guid.NewGuid(), Guid.NewGuid() },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<ArraySegment<byte>>(
                new ArraySegment<byte>[] { bin8Value, bin32Value, new ArraySegment<byte>(new byte[0]) },
                (n1, n2) => { Assert.IsTrue(n1.Count == n2.Count, "Value count not equal."); });

            ArrayTest<string>(
                new string[] { new string('A', 10), new string('B', 300), new string('C', 100) },
                (n1, n2) => { Assert.IsTrue(n1 == n2, "Value not equal."); });

            ArrayTest<AmqpSymbol>(
                new AmqpSymbol[] { new string('A', 10), new string('B', 300), new string('C', 100) },
                (n1, n2) => { Assert.IsTrue(n1.Equals(n2), "Value not equal."); });

            // array of lists
            Guid uuid = Guid.NewGuid();
            List<object> list1 = new List<object>();
            list1.Add(str32Value);
            list1.Add(new AmqpSymbol(strValue));
            list1.Add(uuid);
            list1.Add(8.88d);

            List<object> list2 = new List<object>();
            list2.Add(strValue);
            list2.Add(3333u);

            ArrayTest<IList>(
                new IList[] { list1, list2, list1, list2 },
                (n1, n2) => { EnsureEqual(n1, n2); });
        }

        [TestMethod]
        public void AmqpCodecArrayOfDescribedTest()
        {
            ArrayTest<DescribedType>(
                new DescribedType[] { described1 },
                (n1, n2) => { });

            ArrayTest<DescribedType>(
                new DescribedType[] { described2, described2 },
                (n1, n2) => { });

            ArrayTest<DescribedType>(
                new DescribedType[] { described3, described3, described3 },
                (n1, n2) => { });

            ArrayTest<DescribedType>(
                new DescribedType[] { described4, described4, described4, described4 },
                (n1, n2) => { });
        }

        [TestMethod]
        public void AmqpCodecArraySmallFixedTest()
        {
            // array8: uint0
            {
                byte[] buffer = new byte[] { 0xe0, 0x02, 0x11, 0x43 };
                var array = AmqpCodec.DecodeArray<uint>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x11, array.Length);
                Assert.AreEqual(0u, array[3]);
            }
            // array32: uint0
            {
                byte[] buffer = new byte[] { 0xf0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x11, 0x43 };
                var array = AmqpCodec.DecodeArray<uint>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x11, array.Length);
                Assert.AreEqual(0u, array[3]);
            }
            // array8: ulong0
            {
                byte[] buffer = new byte[] { 0xe0, 0x02, 0x11, 0x44 };
                var array = AmqpCodec.DecodeArray<ulong>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x11, array.Length);
                Assert.AreEqual(0UL, array[3]);
            }
            // array32: ulong0
            {
                byte[] buffer = new byte[] { 0xf0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x11, 0x44 };
                var array = AmqpCodec.DecodeArray<ulong>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x11, array.Length);
                Assert.AreEqual(0UL, array[3]);
            }
            // array8: small-uint
            {
                byte[] buffer = new byte[] { 0xe0, 0x05, 0x03, 0x52, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<uint>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7Fu, array[1]);
            }
            // array32: small-uint
            {
                byte[] buffer = new byte[] { 0xf0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x52, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<uint>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7Fu, array[1]);
            }
            // array8: small-int
            {
                byte[] buffer = new byte[] { 0xe0, 0x05, 0x03, 0x54, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<int>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7F, array[1]);
            }
            // array32: small-int
            {
                byte[] buffer = new byte[] { 0xf0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x54, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<int>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7F, array[1]);
            }
            // array8: small-long
            {
                byte[] buffer = new byte[] { 0xe0, 0x05, 0x03, 0x55, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<long>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7FL, array[1]);
            }
            // array32: small-long
            {
                byte[] buffer = new byte[] { 0xf0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x55, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<long>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7FL, array[1]);
            }
            // array8: small-ulong
            {
                byte[] buffer = new byte[] { 0xe0, 0x05, 0x03, 0x53, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<ulong>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7FUL, array[1]);
            }
            // array32: small-ulong
            {
                byte[] buffer = new byte[] { 0xf0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x53, 0x01, 0x7F, 0xFF };
                var array = AmqpCodec.DecodeArray<ulong>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x03, array.Length);
                Assert.AreEqual(0x7FUL, array[1]);
            }
        }

        [TestMethod]
        public void AmqpCodecArraySmallVariableTest()
        {
            // array8: bin8
            {
                byte[] buffer = new byte[] { 0xe0, 0x09, 0x02, 0xa0, 0x02, 0xaa, 0x0b, 0x03, 0x33, 0x22, 0x00 };
                var array = AmqpCodec.DecodeArray<ArraySegment<byte>>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(0x02, array.Length);
                Assert.AreEqual(2, array[0].Count);
                Assert.AreEqual(3, array[1].Count);
            }
            // array32: str8
            {
                byte[] buffer = new byte[] { 0xe0, 0x09, 0x02, 0xa1, 0x02, 0x98, 0x99, 0x03, 0x33, 0x34, 0x35 };
                var array = AmqpCodec.DecodeArray<string>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(2, array.Length);
                Assert.AreEqual(2, array[0].Length);
                Assert.AreEqual(3, array[1].Length);
            }
            // array8: sym8
            {
                byte[] buffer = new byte[] { 0xe0, 0x09, 0x02, 0xa3, 0x02, 0x98, 0x99, 0x03, 0x33, 0x34, 0x35 };
                var array = AmqpCodec.DecodeArray<AmqpSymbol>(new ByteBuffer(buffer, 0, buffer.Length));
                Assert.AreEqual(2, array.Length);
                Assert.AreEqual(2, array[0].Value.Length);
                Assert.AreEqual(3, array[1].Value.Length);
            }
        }

        [TestMethod]
        public void AmqpSerializerListEncodingTest()
        {
            Action<Person, Person> personValidator = (p1, p2) =>
            {
                Assert.IsNotNull(p2);
                Assert.IsTrue(21 == p2.Age, "Age should be increased by OnDeserialized");
                Assert.AreEqual(p1.GetType().Name, p2.GetType().Name);
                Assert.AreEqual(p1.DateOfBirth.Value, p2.DateOfBirth.Value);
                Assert.AreEqual(p1.Properties.Count, p2.Properties.Count);
                foreach (var k in p1.Properties.Keys)
                {
                    Assert.AreEqual(p1.Properties[k], p2.Properties[k]);
                }
            };

            Action<List<int>, List<int>> gradesValidator = (l1, l2) =>
            {
                if (l1 == null || l2 == null)
                {
                    Assert.IsTrue(l1 == null && l2 == null);
                    return;
                }

                Assert.AreEqual(l1.Count, l2.Count);
                for (int i = 0; i < l1.Count; ++i)
                {
                    Assert.AreEqual(l1[i], l2[i]);
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
            Assert.AreEqual(((Student)p).Address.FullAddress, ((Student)p3).Address.FullAddress);
            gradesValidator(((Student)p).Grades, ((Student)p3).Grades);

            // Inter-op: it should be an AMQP described list as other clients see it
            stream.Seek(0, SeekOrigin.Begin);
            DescribedType dl1 = (DescribedType)AmqpEncoding.DecodeObject(new ByteBuffer(stream.ToArray(), 0, (int)stream.Length));
            Assert.AreEqual(1ul, dl1.Descriptor);
            List<object> lv = dl1.Value as List<object>;
            Assert.IsNotNull(lv);
            Assert.AreEqual(p.Name, lv[0]);
            Assert.AreEqual(p.Age, lv[1]);
            Assert.AreEqual(p.DateOfBirth.Value, lv[2]);
            Assert.IsTrue(lv[3] is DescribedType, "Address is decribed type");
            Assert.AreEqual(3ul, ((DescribedType)lv[3]).Descriptor);
            Assert.AreEqual(((List<object>)((DescribedType)lv[3]).Value)[0], ((Student)p).Address.FullAddress);
            Assert.IsTrue(lv[4] is AmqpMap, "Properties should be map");
            Assert.AreEqual(((AmqpMap)lv[4])[new MapKey("height")], p.Properties["height"]);
            Assert.AreEqual(((AmqpMap)lv[4])[new MapKey("male")], p.Properties["male"]);
            Assert.AreEqual(((AmqpMap)lv[4])[new MapKey("nick-name")], p.Properties["nick-name"]);
            Assert.IsTrue(lv[5] is List<object>);

            // Non-default serializer
            AmqpContractSerializer serializer = new AmqpContractSerializer();
            ByteBuffer bf1 = new ByteBuffer(1024, true);
            serializer.WriteObjectToBuffer(bf1, p);

            Person p4 = serializer.ReadObjectFromBuffer<Person, Person>(bf1);
            personValidator(p, p4);

            // Extensible: more items in the payload should not break
            DescribedType dl2 = new DescribedType(
                new AmqpSymbol("teacher"),
                new List<object>() { "Jerry", 40, null, 50000, lv[4], null, null, "unknown-string", true, new AmqpSymbol("unknown-symbol")});
            ByteBuffer bf2 = new ByteBuffer(1024, true);
            AmqpEncoding.EncodeObject(dl2, bf2);
            AmqpCodec.EncodeULong(100ul, bf2);

            Person p5 = serializer.ReadObjectFromBuffer<Person, Person>(bf2);
            Assert.IsTrue(p5 is Teacher);
            Assert.AreEqual(100ul, AmqpCodec.DecodeULong(bf2));   // unknowns should be skipped
            Assert.AreEqual(0, bf2.Length);

            // teacher
            Teacher teacher = new Teacher("Han");
            teacher.Age = 30;
            teacher.Sallary = 60000;
            teacher.Classes = new Dictionary<int, string>() { { 101, "CS" }, { 102, "Math" }, { 205, "Project" } };

            ByteBuffer bf3 = new ByteBuffer(1024, true);
            serializer.WriteObjectToBuffer(bf3, teacher);

            Person p6 = serializer.ReadObjectFromBuffer<Person, Person>(bf3);
            Assert.IsTrue(p6 is Teacher);
            Assert.AreEqual(teacher.Age + 1, p6.Age);
            Assert.AreEqual(teacher.Sallary * 2, ((Teacher)p6).Sallary);
            Assert.AreEqual(teacher.Id, ((Teacher)p6).Id);
            Assert.AreEqual(teacher.Classes[101], ((Teacher)p6).Classes[101]);
            Assert.AreEqual(teacher.Classes[102], ((Teacher)p6).Classes[102]);
            Assert.AreEqual(teacher.Classes[205], ((Teacher)p6).Classes[205]);
        }

        [TestMethod]
        public void AmqpSerializerMapEncodingTest()
        {
            NamedList<string> list = new NamedList<string>()
            {
                Name = "test-list",
                List = new string[] { "v1", "v2" }
            };

            AmqpContractSerializer serializer = new AmqpContractSerializer();
            ByteBuffer b = new ByteBuffer(1024, true);
            serializer.WriteObjectToBuffer(b, list);

            var result = serializer.ReadObjectFromBuffer<NamedList<string>, NamedList<string>>(b);
            Assert.AreEqual(list.Name, result.Name);
            EnsureEqual((IList)list.List, (IList)result.List);
        }

#if !NETCOREAPP && !WINDOWS_UWP
        [TestMethod]
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
                Assert.IsFalse(object.ReferenceEquals(amqpException1, amqpException2), "Exceptions should not be the same instance!");
                Assert.AreEqual(amqpException1.Message, amqpException2.Message);
                Assert.AreEqual(amqpException1.Error.Condition, amqpException2.Error.Condition);
                Assert.AreEqual(amqpException1.Error.Description, amqpException2.Error.Description);
            }
        }
#endif

        static void EncodeDescribedList(ByteBuffer buffer, object descriptor, params object[] values)
        {
            object descriptor2 = descriptor is string ? (AmqpSymbol)(string)descriptor : descriptor;
            object[] values2 = values ?? new object[] { null };

            DescribedType describedType = new DescribedType(descriptor2, new List<object>(values2));
            AmqpEncoding.EncodeObject(describedType, buffer);

            int size = AmqpEncoding.GetObjectEncodeSize(describedType);
            // testing buffer auto grow

            using (ByteBuffer temp = new ByteBuffer(size / 2, true))
            {
                AmqpEncoding.EncodeObject(describedType, temp);
                AmqpEncoding.DecodeObject(temp);
            }

            // testing encode size
            using (ByteBuffer temp = new ByteBuffer(size, false))
            {
                AmqpEncoding.EncodeObject(describedType, temp);
                AmqpEncoding.DecodeObject(temp);
            }
        }

        static void ArrayTest<T>(T[] array, Action<T, T> validate)
        {
            Debug.WriteLine(string.Format("Array testing for type {0}", typeof(T).ToString()));
            int size = AmqpCodec.GetArrayEncodeSize(array);
            ByteBuffer buffer = new ByteBuffer(128, true);
            AmqpCodec.EncodeArray(array, buffer);
            Assert.IsTrue(buffer.Length <= size);

            T[] decodedArray = AmqpCodec.DecodeArray<T>(buffer);
            Assert.IsTrue(array.Length == decodedArray.Length, "Count not equal.");
            for (int i = 0; i < decodedArray.Length; ++i)
            {
                validate(array[i], decodedArray[i]);
            }
        }

        void EnsureEqual(byte[] data1, int offset1, int count1, byte[] data2, int offset2, int count2)
        {
            Assert.IsTrue(count1 == count2, "Count is not equal.");
            for (int i = 0; i < count1; ++i)
            {
                byte b1 = data1[offset1 + i];
                byte b2 = data2[offset2 + i];
                Assert.IsTrue(b1 == b2, string.Format("The {0}th byte is not equal ({1} != {2}).", i, b1, b2));
            }
        }

        void EnsureEqual(IList list1, IList list2)
        {
            if (list1 == null && list2 == null)
            {
                return;
            }

            Assert.IsTrue(list1 != null && list2 != null, "One of the list is null");

            Assert.IsTrue(list1.Count == list2.Count, "Count not equal.");
            for (int i = 0; i < list1.Count; i++)
            {
                Assert.IsTrue(list1[i].Equals(list2[i]), "Value not equal.");
            }
        }

        void EnsureEqual(DateTime d1, DateTime d2)
        {
            Assert.IsTrue(Math.Abs((d1.ToUniversalTime() - d2.ToUniversalTime()).TotalMilliseconds) < 5, "Datetime difference is greater than 5ms.");
        }

        [TestMethod]
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
                try
                {
                    AmqpCodec.DecodeObject(buffer);
                    Assert.Fail($"[T={typeof(T).Name}, fc={formatCode}] Expected AmqpException from DecodeObject.");
                }
                catch (AmqpException ex)
                {
                    Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
                }
                catch (Exception ex) when (!(ex is UnitTestAssertException))
                {
                    Assert.Fail($"[T={typeof(T).Name}, fc={formatCode}] DecodeObject threw {ex.GetType().Name}: {ex.Message}");
                }
            }

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                try
                {
                    AmqpCodec.DecodeArray<T>(buffer);
                    Assert.Fail($"[T={typeof(T).Name}, fc={formatCode}] Expected AmqpException from DecodeArray<T>.");
                }
                catch (AmqpException ex)
                {
                    Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
                }
                catch (Exception ex) when (!(ex is UnitTestAssertException))
                {
                    Assert.Fail($"[T={typeof(T).Name}, fc={formatCode}] DecodeArray<T> threw {ex.GetType().Name}: {ex.Message}");
                }
            }
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
            {
                Frame frame = new Frame();
                frame.Decode(buffer);
            });
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
        public void AmqpCodecBinaryCountBoundsCheckTest()
        {
            byte[] payload = new byte[] { 0xb0, 0x7f, 0xff, 0xff, 0xff };
            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeBinary(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);

            byte[] payload8 = new byte[] { 0xa0, 0xff };
            buffer = new ByteBuffer(new ArraySegment<byte>(payload8));
            ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeBinary(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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
                AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
                Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeArray<ulong>(buffer));
                Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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
            object result = AmqpCodec.DecodeObject(buffer);

            Assert.IsNotNull(result);
            Array[] arrays = (Array[])result;
            Assert.AreEqual(2, arrays.Length);
            Assert.AreEqual(3, arrays[0].Length);
            Assert.AreEqual(2, arrays[1].Length);
        }

        [TestMethod]
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
                AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeArray<uint>(buffer));
                Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }

            {
                ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
                EncodingBase encoding = AmqpEncoding.GetEncoding(FormatCode.Array32);
                AmqpException ex = Assert.ThrowsException<AmqpException>(() => encoding.DecodeObject(buffer, FormatCode.Array32));
                Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            }
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
        public void AmqpCodecDecimal128CanonicalWireBytesTest()
        {
            // Canonical decimal128 wire bytes.
            // Each test encodes a C# decimal and compares all 17 bytes
            // (format code 0x94 + 16 payload bytes) against hard-coded expected values.

            void AssertEncoded(decimal value, byte[] expectedPayload)
            {
                byte[] expected = new byte[1 + expectedPayload.Length];
                expected[0] = 0x94;
                Array.Copy(expectedPayload, 0, expected, 1, expectedPayload.Length);

                var buffer = new ByteBuffer(new byte[FixedWidth.Decimal128Encoded]);
                AmqpCodec.EncodeDecimal(value, buffer);

                EnsureEqual(expected, 0, expected.Length, buffer.Buffer, buffer.Offset, buffer.Length);
            }

            // 0m
            AssertEncoded(0m, new byte[]
                { 0x30, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

            // 1m
            AssertEncoded(1m, new byte[]
                { 0x30, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 });

            // -1m
            AssertEncoded(-1m, new byte[]
                { 0xB0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 });

            // 1.00m
            AssertEncoded(1.00m, new byte[]
                { 0x30, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64 });

            // decimal.MaxValue
            AssertEncoded(decimal.MaxValue, new byte[]
                { 0x30, 0x40, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

            // decimal.MinValue
            AssertEncoded(decimal.MinValue, new byte[]
                { 0xB0, 0x40, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

            // 0.0000000000000000000000000001m (smallest positive with scale 28)
            AssertEncoded(0.0000000000000000000000000001m, new byte[]
                { 0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 });

            // Negative zero
            decimal negativeZero = new decimal(0, 0, 0, true, 0);
            AssertEncoded(negativeZero, new byte[]
                { 0xB0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        }

        [TestMethod]
        public void AmqpCodecDecimal128PreservesScaleAndSignTest()
        {
            // Values that are numerically equal but have different representations
            // must produce different wire bytes.

            byte[] EncodeToBytes(decimal value)
            {
                var buffer = new ByteBuffer(new byte[FixedWidth.Decimal128Encoded]);
                AmqpCodec.EncodeDecimal(value, buffer);
                byte[] result = new byte[buffer.Length];
                Array.Copy(buffer.Buffer, buffer.Offset, result, 0, buffer.Length);
                return result;
            }

            byte[] one = EncodeToBytes(1m);
            byte[] onePointZero = EncodeToBytes(1.0m);
            byte[] onePointZeroZero = EncodeToBytes(1.00m);

            // All three must produce different wire representations
            Assert.AreNotEqual(one.Length, 0);
            bool oneEqOneZero = true;
            bool oneEqOneZeroZero = true;
            bool oneZeroEqOneZeroZero = true;
            for (int i = 0; i < one.Length; i++)
            {
                if (one[i] != onePointZero[i]) oneEqOneZero = false;
                if (one[i] != onePointZeroZero[i]) oneEqOneZeroZero = false;
                if (onePointZero[i] != onePointZeroZero[i]) oneZeroEqOneZeroZero = false;
            }
            Assert.IsFalse(oneEqOneZero, "1m and 1.0m must differ");
            Assert.IsFalse(oneEqOneZeroZero, "1m and 1.00m must differ");
            Assert.IsFalse(oneZeroEqOneZeroZero, "1.0m and 1.00m must differ");

            // Verify sign and scale using decimal.GetBits as reference
            int[] bits1 = decimal.GetBits(1m);
            int[] bits10 = decimal.GetBits(1.0m);
            int[] bits100 = decimal.GetBits(1.00m);

            // All positive (sign bit in bits[3] bit 31)
            Assert.IsTrue((bits1[3] & unchecked((int)0x80000000)) == 0);
            Assert.IsTrue((bits10[3] & unchecked((int)0x80000000)) == 0);
            Assert.IsTrue((bits100[3] & unchecked((int)0x80000000)) == 0);

            // Scales differ: 0, 1, 2
            Assert.AreEqual(0, (bits1[3] >> 16) & 0xFF);
            Assert.AreEqual(1, (bits10[3] >> 16) & 0xFF);
            Assert.AreEqual(2, (bits100[3] >> 16) & 0xFF);
        }

        [TestMethod]
        public void AmqpCodecDecimal128RoundTripTest()
        {
            void AssertRoundTrip(decimal value)
            {
                var buffer = new ByteBuffer(new byte[FixedWidth.Decimal128Encoded]);
                AmqpCodec.EncodeDecimal(value, buffer);
                decimal decoded = AmqpCodec.DecodeDecimal(buffer).Value;
                Assert.AreEqual(value, decoded);
            }

            // Zero and negative zero
            AssertRoundTrip(0m);
            AssertRoundTrip(new decimal(0, 0, 0, true, 0));

            // Positive and negative values
            AssertRoundTrip(1m);
            AssertRoundTrip(-1m);
            AssertRoundTrip(42m);
            AssertRoundTrip(-42m);

            // Various scales
            AssertRoundTrip(1.0m);
            AssertRoundTrip(1.00m);
            AssertRoundTrip(3.14159m);
            AssertRoundTrip(0.0000000000000000000000000001m);

            // Min/Max
            AssertRoundTrip(decimal.MinValue);
            AssertRoundTrip(decimal.MaxValue);

            // Values with all three coefficient words non-zero
            AssertRoundTrip(1234567890123456789012345678m);
            AssertRoundTrip(-1234567890123456789012345678m);
        }

        [TestMethod]
        public void AmqpCodecDecimal128ArrayRoundTripTest()
        {
            decimal[] values = new decimal[]
            {
                decimal.MinValue,
                -234934.092348m,
                0m,
                new decimal(0, 0, 0, true, 0), // negative zero
                38743947394.2349324m,
                decimal.MaxValue
            };

            int size = AmqpCodec.GetArrayEncodeSize(values);
            ByteBuffer buffer = new ByteBuffer(size, true);
            AmqpCodec.EncodeArray(values, buffer);

            decimal[] decoded = AmqpCodec.DecodeArray<decimal>(buffer);
            Assert.AreEqual(values.Length, decoded.Length);
            for (int i = 0; i < values.Length; i++)
            {
                Assert.AreEqual(values[i], decoded[i]);
            }
        }

        [TestMethod]
        public void AmqpCodecDecimal128ExistingFixturesTest()
        {
            // Existing decode fixtures must continue to work.
            decimal? dec32 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal32ValueBin)));
            Assert.IsTrue(dec32.Value == decimal32Value, "Decimal32 value is not equal");

            decimal? dec64 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal64ValueBin)));
            Assert.IsTrue(dec64.Value == decimal64Value, "Decimal64 value is not equal");

            decimal? dec128 = AmqpCodec.DecodeDecimal(new ByteBuffer(new ArraySegment<byte>(decimal128ValueBin)));
            Assert.IsTrue(dec128.Value == decimal128Value, "Decimal128 value is not equal");
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(inner))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            Assert.AreEqual(itemCount, result.Length);
            for (int i = 0; i < itemCount; i++)
            {
                Assert.AreEqual((uint)i, result[i]);
            }
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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

            // Expected: some exception (buffer underflow or budget exceeded).
            ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(payload));
            try
            {
                AmqpCodec.DecodeObject(buffer);
                Assert.Fail("Expected an exception.");
            }
            catch (Exception ex) when (!(ex is UnitTestAssertException))
            {
            }
        }

        [TestMethod]
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

            Assert.IsNotNull(result);
            uint[] arr = (uint[])result;
            Assert.AreEqual(count, arr.Length);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpEncoding.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
        public void AmqpCodecDecodeObjectFormatCodeOverloadTest()
        {
            // Regression test: DecodeObject(buffer, formatCode) must dispatch via FormatCode
            // overload, NOT interpret formatCode as depth.
            ByteBuffer bf = new ByteBuffer(256, true);
            AmqpCodec.EncodeList(new List<object> { "hello", 42 }, bf);

            FormatCode formatCode = AmqpEncoding.ReadFormatCode(bf);
            object result = AmqpEncoding.DecodeObject(bf, formatCode);

            Assert.IsNotNull(result);
            List<object> list = result as List<object>;
            Assert.IsNotNull(list);
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual("hello", list[0]);
            Assert.AreEqual(42, list[1]);
        }

        [TestMethod]
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
            Assert.IsNotNull(decoded);
            Assert.AreEqual(itemCount, decoded.Count);
            for (int i = 0; i < itemCount; i++)
            {
                Assert.AreEqual(i, decoded[i]);
            }
        }

        [TestMethod]
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
            Assert.IsNotNull(decoded);
            Assert.AreEqual(entryCount, decoded.Count);
        }

        [TestMethod]
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
            Assert.IsNotNull(decoded);
            Assert.AreEqual(itemCount, decoded.Count);
        }

        [TestMethod]
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
            Assert.IsNotNull(decoded);
            Assert.AreEqual(count, decoded.Count);
            for (int i = 0; i < count; i++)
            {
                DescribedType dt = decoded[i] as DescribedType;
                Assert.IsNotNull(dt);
                Assert.AreEqual((ulong)i, dt.Descriptor);
                Assert.AreEqual("value" + i, dt.Value);
            }
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
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

            Assert.AreEqual(count, decoded.Length);
            for (int i = 0; i < count; i++)
            {
                Assert.AreEqual(original[i], decoded[i]);
            }
        }

        [TestMethod]
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
            AmqpException ex = Assert.ThrowsException<AmqpException>(() => AmqpCodec.DecodeObject(buffer));
            Assert.AreEqual((AmqpSymbol)AmqpErrorCode.DecodeError, (AmqpSymbol)ex.Error.Condition);
        }

        [TestMethod]
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

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }

        [TestMethod]
        public void AmqpCodecArrayOfDescribedUnboundedCrossItemTest()
        {
            // List containing:
            //   1) Array<UInt0> with 15000 items (credits 60000 bytes to outer tracker)
            //   2) Array<Described>[1 item, value = Array<UInt0> with 15000 items]
            //      The Array<Described> path must not reset the unbounded tracker for
            //      its items, otherwise the 60000 bytes decoded inside are hidden from
            //      the outer 60000 credit.
            //   Combined actual = 120000 bytes > MaxUnboundedSize (65536) — must throw.

            int perArrayCount = 15000;

            // Inner Array<UInt0> body (following an already-consumed Array32 FC):
            //   size(4) + count(4) + inner-fc(1) = 9 bytes
            byte[] innerBody = new byte[9];
            innerBody[0] = 0; innerBody[1] = 0; innerBody[2] = 0; innerBody[3] = 5;
            innerBody[4] = (byte)(perArrayCount >> 24);
            innerBody[5] = (byte)(perArrayCount >> 16);
            innerBody[6] = (byte)(perArrayCount >> 8);
            innerBody[7] = (byte)perArrayCount;
            innerBody[8] = FormatCode.UInt0;

            // Item 1: full Array<UInt0>
            byte[] item1 = new byte[10];
            item1[0] = FormatCode.Array32;
            Array.Copy(innerBody, 0, item1, 1, 9);

            // Item 2: Array<Described>[1 item, value = Array<UInt0>]
            //   fc(1) + size(4) + count(4) + inner-fc=Described(1)
            //   + descriptor(SmallULong 2 bytes) + value-fc(1) + inner value body(9)
            //   = 22 bytes total; size field = count(4) + Described-fc(1) + payload(12) = 17
            byte[] item2 = new byte[22];
            int off = 0;
            item2[off++] = FormatCode.Array32;
            item2[off++] = 0; item2[off++] = 0; item2[off++] = 0; item2[off++] = 17;
            item2[off++] = 0; item2[off++] = 0; item2[off++] = 0; item2[off++] = 1;
            item2[off++] = FormatCode.Described;
            item2[off++] = FormatCode.SmallULong;
            item2[off++] = 0x09;
            item2[off++] = FormatCode.Array32;
            Array.Copy(innerBody, 0, item2, off, 9);

            int listBodySize = item1.Length + item2.Length;
            byte[] payload = new byte[3 + listBodySize];
            off = 0;
            payload[off++] = FormatCode.List8;
            payload[off++] = (byte)(1 + listBodySize);
            payload[off++] = 2;
            Array.Copy(item1, 0, payload, off, item1.Length);
            off += item1.Length;
            Array.Copy(item2, 0, payload, off, item2.Length);

            AmqpException ex = Assert.ThrowsException<AmqpException>(() =>
                AmqpCodec.DecodeObject(new ByteBuffer(new ArraySegment<byte>(payload))));
            StringAssert.Contains(ex.Error.Description, "unbounded");
        }
    }
}
