// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Diagnostics;

    sealed class ArrayEncoding : EncodingBase<Array>
    {
        public const int PrefixSize = FixedWidth.FormatCode + FixedWidth.Int + FixedWidth.Int + FixedWidth.FormatCode;

        public ArrayEncoding()
            : base(FormatCode.Array32)
        {
        }

        public static int GetEncodeSize<T>(T[] array, EncodingBase<T> encoding)
        {
            Debug.Assert(array != null);
            return ArrayEncoding.PrefixSize + encoding.GetArrayValueSize(array);
        }

        public static void Encode<T>(ByteBuffer buffer, T[] array, EncodingBase<T> encoding)
        {
            Debug.Assert(array != null);
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Array32);
            var sizeTracker = SizeTracker.Track(buffer);
            AmqpBitConverter.WriteInt(buffer, FixedWidth.Int);
            AmqpBitConverter.WriteInt(buffer, array.Length);
            AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);
            encoding.WriteArrayValue(array, buffer);
            sizeTracker.CommitExclusive(0);
        }

        public static T[] Decode<T>(ByteBuffer buffer, FormatCode formatCode, EncodingBase<T> encoding)
        {
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out var size, out var count);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (!object.ReferenceEquals(encoding, AmqpEncoding.GetEncoding(formatCode)))
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, $"Format code '{formatCode}' is different from expected '{encoding.FormatCode}'.");
            }

            return encoding.ReadArrayValue(buffer, formatCode, new T[count]);
        }

        protected override int OnGetSize(Array value, int arrayIndex)
        {
            IEncoding encoding = AmqpEncoding.GetEncoding(value.GetType().GetElementType());
            return encoding.GetArraySize(value);
        }

        protected override void OnWrite(Array value, ByteBuffer buffer, int arrayIndex)
        {
            IEncoding encoding = AmqpEncoding.GetEncoding(value.GetType().GetElementType());
            encoding.WriteArray(value, buffer);
        }

        protected override Array OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out var size, out var count);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            IEncoding encoding = AmqpEncoding.GetEncoding(formatCode);
            return encoding.ReadArray(buffer, formatCode, count);
        }
    }
}
