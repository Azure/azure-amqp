// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class SymbolEncoding : EncodingBase
    {
        public SymbolEncoding()
            : base(FormatCode.Symbol32)
        {
        }

        public static int GetValueSize(AmqpSymbol value)
        {
            return value.Value == null ? FixedWidth.Null : SystemEncoding.ASCII.GetByteCount(value.Value);
        }

        public static int GetEncodeSize(AmqpSymbol value)
        {
            return value.Value == null ?
                FixedWidth.NullEncoded :
                FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(value.ValueSize) + value.ValueSize;
        }

        public static void Encode(AmqpSymbol value, ByteBuffer buffer)
        {
            if (value.Value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                int stringSize = SystemEncoding.ASCII.GetByteCount(value.Value);
                int encodeWidth = AmqpEncoding.GetEncodeWidthBySize(stringSize);
                if (encodeWidth == FixedWidth.UByte)
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Symbol8);
                    AmqpBitConverter.WriteUByte(buffer, (byte)stringSize);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Symbol32);
                    AmqpBitConverter.WriteUInt(buffer, (uint)stringSize);
                }

                buffer.Validate(true, stringSize);
                int bytes = SystemEncoding.ASCII.GetBytes(value.Value, 0, value.Value.Length, buffer.Buffer, buffer.WritePos);
                Fx.Assert(bytes == stringSize, "size wrong");
                buffer.Append(stringSize);
            }
        }

        public static AmqpSymbol Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return new AmqpSymbol();
            }

            int count;
            AmqpEncoding.ReadCount(buffer, formatCode, FormatCode.Symbol8, FormatCode.Symbol32, out count);
            var segment = new ArraySegment<byte>(buffer.Buffer, buffer.Offset, count);
            var symbol = EncodingCache.GetSymbol(segment);
            buffer.Complete(count);

            return symbol;
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.UInt + SystemEncoding.ASCII.GetByteCount(((AmqpSymbol)value).Value);
            }
            else
            {
                return SymbolEncoding.GetEncodeSize((AmqpSymbol)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                string strValue = ((AmqpSymbol)value).Value;
                int stringSize = SystemEncoding.ASCII.GetByteCount(strValue);
                AmqpBitConverter.WriteUInt(buffer, (uint)stringSize);

                buffer.Validate(true, stringSize);
                int bytes = SystemEncoding.ASCII.GetBytes(strValue, 0, strValue.Length, buffer.Buffer, buffer.WritePos);
                Fx.Assert(bytes == stringSize, "size wrong");
                buffer.Append(stringSize);
            }
            else
            {
                SymbolEncoding.Encode((AmqpSymbol)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            AmqpSymbol symbol = SymbolEncoding.Decode(buffer, formatCode);
            return EncodingCache.Box(symbol);
        }
    }
}
