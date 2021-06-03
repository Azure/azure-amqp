// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Buffers.Binary;
    using System.Collections.Generic;

    sealed class TimeStampEncoding : PrimitiveEncoding<DateTime>
    {
        static readonly long MaxMilliseconds = (long)(DateTime.MaxValue.ToUniversalTime() - AmqpConstants.StartOfEpoch).TotalMilliseconds;

        public TimeStampEncoding()
            : base(FormatCode.TimeStamp)
        {
        }

        public static int GetEncodeSize(DateTime? value)
        {
            return value.HasValue ? FixedWidth.TimeStampEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(DateTime? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.TimeStamp);
                AmqpBitConverter.WriteLong(buffer, GetMilliseconds(value.Value));
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static DateTime? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return ToDateTime(AmqpBitConverter.ReadLong(buffer));
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.TimeStamp : GetEncodeSize((DateTime)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteLong(buffer, GetMilliseconds((DateTime)value));
            }
            else
            {
                Encode((DateTime)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return TimeStampEncoding.Decode(buffer, formatCode);
        }

        private static long GetMilliseconds(DateTime value)
        {
            DateTime utcValue = value.ToUniversalTime();
            double millisends = (utcValue - AmqpConstants.StartOfEpoch).TotalMilliseconds;
            return (long)millisends;
        }

        private static DateTime ToDateTime(long milliseconds)
        {
            milliseconds = milliseconds < 0 ? 0 : milliseconds;
            return milliseconds >= MaxMilliseconds ? DateTime.MaxValue : AmqpConstants.StartOfEpoch.AddMilliseconds(milliseconds);
        }

        public override int GetArrayEncodeSize(IList<DateTime> value)
        {
            return FixedWidth.TimeStamp * value.Count;
        }

        public override void EncodeArray(IList<DateTime> value, ByteBuffer buffer)
        {
            int byteCount = FixedWidth.TimeStamp * value.Count;

            buffer.Validate(write: true, byteCount);

            Span<byte> destination = buffer.GetWriteSpan();

            if (value is DateTime[] dateTimeArray)
            {
                // fast-path for long[] so the bounds checks can be elided
                for (int i = 0; i < dateTimeArray.Length; i++)
                {
                    BinaryPrimitives.WriteInt64BigEndian(destination.Slice(FixedWidth.TimeStamp * i), GetMilliseconds(dateTimeArray[i]));
                }
            }
            else
            {
                IReadOnlyList<DateTime> listValue = (IReadOnlyList<DateTime>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    BinaryPrimitives.WriteInt64BigEndian(destination.Slice(FixedWidth.TimeStamp * i), GetMilliseconds(listValue[i]));
                }
            }

            buffer.Append(byteCount);
        }

        public override DateTime[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            int byteCount = FixedWidth.TimeStamp * count;
            buffer.Validate(write: false, byteCount);
            ReadOnlySpan<byte> source = buffer.GetReadSpan();

            DateTime[] array = new DateTime[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = ToDateTime(BinaryPrimitives.ReadInt64BigEndian(source.Slice(FixedWidth.TimeStamp * i)));
            }

            buffer.Complete(byteCount);

            return array;
        }
    }
}
