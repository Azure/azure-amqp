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

        internal static T[] Decode<T>(ByteBuffer buffer, FormatCode formatCode, EncodingBase<T> encoding, int depth, ref int totalUnboundedSize)
        {
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out var size, out var count);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (!object.ReferenceEquals(encoding, AmqpEncoding.GetEncoding(formatCode)))
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, $"Format code '{formatCode}' is different from expected '{encoding.FormatCode}'.");
            }

            return (T[])encoding.DecodeArray(buffer, formatCode, count, depth, ref totalUnboundedSize);
        }

        protected override int OnGetSize(Array value, int arrayIndex)
        {
            int valueSize = 0;
            if (value.Length > 0)
            {
                EncodingBase encoding = AmqpEncoding.GetEncoding(value.GetValue(0).GetType());
                valueSize = encoding.GetArrayEncodeSize(value);
            }

            return PrefixSize + valueSize;
        }

        protected override void OnWrite(Array value, ByteBuffer buffer, int arrayIndex)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Array32);
            var sizeTracker = SizeTracker.Track(buffer);
            AmqpBitConverter.WriteInt(buffer, FixedWidth.Int);
            AmqpBitConverter.WriteInt(buffer, value.Length);

            if (value.Length > 0)
            {
                EncodingBase encoding = AmqpEncoding.GetEncoding(value.GetValue(0).GetType());
                AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);
                encoding.EncodeArray(value, buffer);
            }

            sizeTracker.CommitExclusive(0);
        }

        protected override Array OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            int totalUnboundedSize = 0;
            return DecodeArrayInternal(buffer, formatCode, 0, ref totalUnboundedSize);
        }

        internal override object DecodeObject(ByteBuffer buffer, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            return DecodeArrayInternal(buffer, formatCode, depth, ref totalUnboundedSize);
        }

        Array DecodeArrayInternal(ByteBuffer buffer, FormatCode formatCode, int depth, ref int totalUnboundedSize)
        {
            AmqpEncoding.CheckMaxNestingDepth(depth);
            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out var size, out var count);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            EncodingBase encoding = AmqpEncoding.GetEncoding(formatCode);
            return encoding.DecodeArray(buffer, formatCode, count, depth + 1, ref totalUnboundedSize);
        }
    }
}
