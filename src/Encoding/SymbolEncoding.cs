// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers;
    using System.Collections.Generic;
    using System.Text;

    sealed class SymbolEncoding : PrimitiveEncoding<AmqpSymbol>
    {
        public SymbolEncoding()
            : base(FormatCode.Symbol32)
        {
        }

        public static int GetValueSize(AmqpSymbol value)
        {
            return value.Value == null ? FixedWidth.Null : Encoding.ASCII.GetByteCount(value.Value);
        }

        public static int GetEncodeSize(AmqpSymbol value)
        {
            if (value.Value == null)
            {
                return FixedWidth.NullEncoded;
            }

            int valueSize = GetValueSize(value);
            return FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(valueSize) + valueSize;
        }

        public static void Encode(AmqpSymbol value, ByteBuffer buffer)
        {
            if (value.Value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                ReadOnlySpan<byte> encodedData = Encoding.ASCII.GetBytes(value.Value);
                int encodeWidth = AmqpEncoding.GetEncodeWidthBySize(encodedData.Length);
                AmqpBitConverter.WriteUByte(buffer, encodeWidth == FixedWidth.UByte ? FormatCode.Symbol8 : FormatCode.Symbol32);
                SymbolEncoding.Encode(encodedData, encodeWidth, buffer);
            }
        }

        public static AmqpSymbol Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return new AmqpSymbol();
            }

            AmqpEncoding.ReadCount(buffer, formatCode, FormatCode.Symbol8, FormatCode.Symbol32, out var count);
            string value = Encoding.ASCII.GetString(buffer.Buffer, buffer.Offset, count);
            buffer.Complete(count);

            return new AmqpSymbol(value);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.UInt + Encoding.ASCII.GetByteCount(((AmqpSymbol)value).Value);
            }

            return SymbolEncoding.GetEncodeSize((AmqpSymbol)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                SymbolEncoding.Encode(Encoding.ASCII.GetBytes(((AmqpSymbol)value).Value), FixedWidth.UInt, buffer);
            }
            else
            {
                SymbolEncoding.Encode((AmqpSymbol)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return SymbolEncoding.Decode(buffer, formatCode);
        }

        static void Encode(ReadOnlySpan<byte> encodedData, int width, ByteBuffer buffer)
        {
            if (width == FixedWidth.UByte)
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)encodedData.Length);
            }
            else
            {
                AmqpBitConverter.WriteUInt(buffer, (uint)encodedData.Length);
            }

            AmqpBitConverter.WriteBytes(buffer, encodedData, 0, encodedData.Length);
        }

        public override int GetArrayEncodeSize(IList<AmqpSymbol> value)
        {
            IReadOnlyList<AmqpSymbol> listValue = (IReadOnlyList<AmqpSymbol>)value;

            int size = 0;
            for (var index = 0; index < listValue.Count; index++)
            {
                AmqpSymbol item = listValue[index];
                size += Encoding.ASCII.GetByteCount(item.Value);
            }
            return size;
        }

        public override void EncodeArray(IList<AmqpSymbol> value, ByteBuffer buffer)
        {
            for (var index = 0; index < value.Count; index++)
            {
                AmqpSymbol item = value[index];
                string stringValue = item.Value;
                int byteCount = Encoding.ASCII.GetByteCount(stringValue);

                var pool = ArrayPool<byte>.Shared;
                var tempBuffer = pool.Rent(byteCount);

                int encodedByteCount = Encoding.ASCII.GetBytes(stringValue, 0, stringValue.Length, tempBuffer, 0);
                AmqpBitConverter.WriteUInt(buffer, (uint)encodedByteCount);
                AmqpBitConverter.WriteBytes(buffer, tempBuffer, 0, encodedByteCount);

                pool.Return(tempBuffer);
            }
        }

        public override AmqpSymbol[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            AmqpSymbol[] array = new AmqpSymbol[count];
            for (int i = 0; i < count; ++i)
            {
                var length = (int) AmqpBitConverter.ReadUInt(buffer);
                array[i] = Encoding.ASCII.GetString(buffer.Buffer, buffer.Offset, length);
                buffer.Complete(length);
            }
            return array;
        }
    }
}
