// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Diagnostics;
    using System.Text;

    sealed class SymbolEncoding : EncodingBase<AmqpSymbol>
    {
        public SymbolEncoding()
            : base(FormatCode.Symbol32)
        {
        }

        public static int GetEncodeSize(AmqpSymbol value)
        {
            if (value.Value == null)
            {
                return FixedWidth.NullEncoded;
            }

            int byteCount = Encoding.ASCII.GetByteCount(value.Value);
            return FixedWidth.FormatCode + AmqpEncoding.GetEncodeWidthBySize(byteCount) + byteCount;
        }

        public static void Encode(AmqpSymbol value, ByteBuffer buffer)
        {
            if (value.Value == null)
            {
                AmqpEncoding.EncodeNull(buffer);
                return;
            }

            int byteCount = Encoding.ASCII.GetByteCount(value.Value);
            if (byteCount <= byte.MaxValue)
            {
                AmqpBitConverter.Write(buffer, FormatCode.Symbol8, (byte)byteCount);
            }
            else
            {
                AmqpBitConverter.Write(buffer, FormatCode.Symbol32, (uint)byteCount);
            }

            buffer.ValidateWrite(byteCount);
            int encodedByteCount = Encoding.ASCII.GetBytes(value.Value, 0, value.Value.Length, buffer.Buffer, buffer.WritePos);
            Debug.Assert(byteCount == encodedByteCount);
            buffer.Append(encodedByteCount);
        }

        public static AmqpSymbol Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            int length;
            if (formatCode == FormatCode.Symbol8)
            {
                length = (int)AmqpBitConverter.ReadUByte(buffer);
            }
            else if (formatCode == FormatCode.Symbol32)
            {
                length = AmqpBitConverter.ReadInt(buffer);
            }
            else
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }

            buffer.ValidateRead(length);
            string value = Encoding.ASCII.GetString(buffer.Buffer, buffer.Offset, length);
            buffer.Complete(length);
            return value;
        }

        public override int GetArrayValueSize(AmqpSymbol[] array)
        {
            int size = 0;
            for (int i = 0; i < array.Length; i++)
            {
                size += this.OnGetSize(array[i], i);
            }

            return size;
        }

        public override void WriteArrayValue(AmqpSymbol[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                this.OnWrite(array[i], buffer, i);
            }
        }

        public override AmqpSymbol[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, AmqpSymbol[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = Decode(buffer, formatCode);
            }

            return array;
        }

        protected override int OnGetSize(AmqpSymbol value, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                return GetEncodeSize(value);
            }

            ValidateArrayItem(value);
            return FixedWidth.Int + Encoding.ASCII.GetByteCount(value.Value);
        }

        protected override void OnWrite(AmqpSymbol value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                ValidateArrayItem(value);
                int byteCount = Encoding.ASCII.GetByteCount(value.Value);
                AmqpBitConverter.WriteInt(buffer, byteCount);
                buffer.ValidateWrite(byteCount);
                int count = Encoding.ASCII.GetBytes(value.Value, 0, value.Value.Length, buffer.Buffer, buffer.WritePos);
                buffer.Append(count);
            }
        }

        protected override AmqpSymbol OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }

        static void ValidateArrayItem(AmqpSymbol value)
        {
            if (value.Value == null)
            {
                throw new ArgumentNullException("Array cannot have null symbols.");
            }
        }
    }
}
