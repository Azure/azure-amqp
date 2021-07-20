// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class TimeStampEncoding : EncodingBase<DateTime>
    {
        static readonly long MaxMilliseconds = (long)(DateTime.MaxValue.ToUniversalTime() - AmqpConstants.StartOfEpoch).TotalMilliseconds;

        public TimeStampEncoding()
            : base(FormatCode.TimeStamp, FixedWidth.TimeStamp)
        {
        }

        public static int GetEncodeSize(DateTime value)
        {
            return FixedWidth.TimeStampEncoded;
        }

        public static void Encode(DateTime value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.TimeStamp);
            AmqpBitConverter.WriteLong(buffer, GetMilliseconds(value));
        }

        public static DateTime Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return ToDateTime(AmqpBitConverter.ReadLong(buffer));
        }

        public override int GetArrayValueSize(DateTime[] array)
        {
            return array.Length * FixedWidth.TimeStamp;
        }

        public override void WriteArrayValue(DateTime[] array, ByteBuffer buffer)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateWrite(size);
            for (int i = 0, pos = buffer.WritePos; i < array.Length; i++, pos += FixedWidth.Long)
            {
                AmqpBitConverter.WriteULong(buffer.Buffer, pos, (ulong)GetMilliseconds(array[i]));
            }

            buffer.Append(size);
        }

        public override DateTime[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, DateTime[] array)
        {
            int size = this.GetArrayValueSize(array);
            buffer.ValidateRead(size);
            for (int i = 0, pos = buffer.Offset; i < array.Length; i++, pos += FixedWidth.Long)
            {
                long data = (long)AmqpBitConverter.ReadULong(buffer.Buffer, pos, FixedWidth.Long);
                array[i] = ToDateTime(data);
            }

            buffer.Complete(size);
            return array;
        }

        protected override int OnGetSize(DateTime value, int arrayIndex)
        {
            return arrayIndex < 0 ? FixedWidth.TimeStampEncoded : FixedWidth.TimeStamp;
        }

        protected override void OnWrite(DateTime value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteLong(buffer, GetMilliseconds(value));
            }
        }

        protected override DateTime OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }

        static long GetMilliseconds(DateTime value)
        {
            DateTime utcValue = value.ToUniversalTime();
            double millisends = (utcValue - AmqpConstants.StartOfEpoch).TotalMilliseconds;
            return (long)millisends;
        }

        static DateTime ToDateTime(long milliseconds)
        {
            milliseconds = milliseconds < 0 ? 0 : milliseconds;
            return milliseconds >= MaxMilliseconds ? DateTime.MaxValue : AmqpConstants.StartOfEpoch.AddMilliseconds(milliseconds);
        }
    }
}
