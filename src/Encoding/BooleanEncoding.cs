// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    sealed class BooleanEncoding : PrimitiveEncoding<bool>
    {
        public BooleanEncoding()
            : base(FormatCode.Boolean)
        {
        }

        public static int GetEncodeSize(bool? value)
        {
            return value.HasValue ? FixedWidth.BooleanEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(bool? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, value.Value ? FormatCode.BooleanTrue : FormatCode.BooleanFalse);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static bool? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            VerifyFormatCode(formatCode, buffer.Offset, FormatCode.Boolean, FormatCode.BooleanFalse, FormatCode.BooleanTrue);
            if (formatCode == FormatCode.Boolean)
            {
                return AmqpBitConverter.ReadUByte(buffer) != 0;
            }

            return formatCode == FormatCode.BooleanTrue ? true : false;
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            if (arrayEncoding)
            {
                return FixedWidth.BooleanVar;
            }

            return GetEncodeSize((bool)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)((bool)value ? 1 : 0));
            }
            else
            {
                Encode((bool)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<bool> value)
        {
            return FixedWidth.BooleanVar * value.Count;
        }

        public override void EncodeArray(IList<bool> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.BooleanVar * value.Count;
            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is bool[] boolArray)
            {
                // fast-path for int[] so the bounds checks can be elided
                for (int i = 0; i < boolArray.Length; i++)
                {
                    destination.Slice(FixedWidth.BooleanVar * i)[0] = (byte) (boolArray[i] ? 1 : 0);
                }
            }
            else
            {
                IReadOnlyList<bool> listValue = (IReadOnlyList<bool>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    destination.Slice(FixedWidth.BooleanVar * i)[0] = (byte) (listValue[i] ? 1 : 0);
                }
            }

            buffer.Append(byteCount);
        }

        public override bool[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.BooleanVar * count;

            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            bool[] array = new bool[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = source.Slice(FixedWidth.BooleanVar * i)[0] == 1;
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
