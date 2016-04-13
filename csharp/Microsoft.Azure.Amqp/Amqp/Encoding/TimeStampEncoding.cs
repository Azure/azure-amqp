// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class TimeStampEncoding : EncodingBase
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
                AmqpBitConverter.WriteLong(buffer, TimeStampEncoding.GetMilliseconds(value.Value));
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
            if (arrayEncoding)
            {
                return FixedWidth.TimeStamp;
            }
            else
            {
                return TimeStampEncoding.GetEncodeSize((DateTime)value);
            }
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteLong(buffer, TimeStampEncoding.GetMilliseconds((DateTime)value));
            }
            else
            {
                TimeStampEncoding.Encode((DateTime)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return TimeStampEncoding.Decode(buffer, formatCode);
        }

        public static long GetMilliseconds(DateTime value)
        {
            DateTime utcValue = value.ToUniversalTime();
            double millisends = (utcValue - AmqpConstants.StartOfEpoch).TotalMilliseconds;
            return (long)millisends;
        }

        public static DateTime ToDateTime(long milliseconds)
        {
            milliseconds = milliseconds < 0 ? 0 : milliseconds;
            if (milliseconds >= MaxMilliseconds)
            {
                return DateTime.MaxValue;
            }

            return AmqpConstants.StartOfEpoch.AddMilliseconds(milliseconds);
        }
    }
}
