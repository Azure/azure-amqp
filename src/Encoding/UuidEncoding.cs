// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;

    sealed class UuidEncoding : EncodingBase<Guid>
    {
        public UuidEncoding()
            : base(FormatCode.Uuid)
        {
        }

        public static int GetEncodeSize(Guid value)
        {
            return FixedWidth.UuidEncoded;
        }

        public static void Encode(Guid value, ByteBuffer buffer)
        {
            AmqpBitConverter.WriteUByte(buffer, FormatCode.Uuid);
            AmqpBitConverter.WriteUuid(buffer, value);
        }

        public static Guid Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            return AmqpBitConverter.ReadUuid(buffer);
        }

        public override int GetArrayValueSize(Guid[] array)
        {
            return FixedWidth.Uuid * array.Length;
        }

        public override void WriteArrayValue(Guid[] array, ByteBuffer buffer)
        {
            for (int i = 0; i < array.Length; i++)
            {
                AmqpBitConverter.WriteUuid(buffer, array[i]);
            }
        }

        public override Guid[] ReadArrayValue(ByteBuffer buffer, FormatCode formatCode, Guid[] array)
        {
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = Decode(buffer, formatCode);
            }

            return array;
        }

        protected override int OnGetSize(Guid value, int arrayIndex)
        {
            return arrayIndex < 0 ? GetEncodeSize(value) : FixedWidth.Uuid;
        }

        protected override void OnWrite(Guid value, ByteBuffer buffer, int arrayIndex)
        {
            if (arrayIndex < 0)
            {
                Encode(value, buffer);
            }
            else
            {
                AmqpBitConverter.WriteUuid(buffer, value);
            }
        }

        protected override Guid OnRead(ByteBuffer buffer, FormatCode formatCode)
        {
            return Decode(buffer, formatCode);
        }
    }
}
