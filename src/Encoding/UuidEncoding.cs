// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    sealed class UuidEncoding : PrimitiveEncoding<Guid>
    {
        public UuidEncoding()
            : base(FormatCode.Uuid)
        {
        }

        public static int GetEncodeSize(Guid? value)
        {
            return value.HasValue ? FixedWidth.UuidEncoded : FixedWidth.NullEncoded;
        }

        public static void Encode(Guid? value, ByteBuffer buffer)
        {
            if (value.HasValue)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Uuid);
                AmqpBitConverter.WriteUuid(buffer, value.Value);
            }
            else
            {
                AmqpEncoding.EncodeNull(buffer);
            }
        }

        public static Guid? Decode(ByteBuffer buffer, FormatCode formatCode)
        {
            if (formatCode == 0 && (formatCode = AmqpEncoding.ReadFormatCode(buffer)) == FormatCode.Null)
            {
                return null;
            }

            return AmqpBitConverter.ReadUuid(buffer);
        }

        public override int GetObjectEncodeSize(object value, bool arrayEncoding)
        {
            return arrayEncoding ? FixedWidth.Uuid : UuidEncoding.GetEncodeSize((Guid)value);
        }

        public override void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer)
        {
            if (arrayEncoding)
            {
                AmqpBitConverter.WriteUuid(buffer, (Guid)value);
            }
            else
            {
                UuidEncoding.Encode((Guid)value, buffer);
            }
        }

        public override object DecodeObject(ByteBuffer buffer, FormatCode formatCode)
        {
            return UuidEncoding.Decode(buffer, formatCode);
        }

        public override int GetArrayEncodeSize(IList<Guid> value)
        {
            return FixedWidth.Uuid * value.Count;
        }

        public override void EncodeArray(IList<Guid> value, ByteBuffer buffer)
        {
            if (value is Guid[] guidArray)
            {
                // fast-path for ushort[] so the bounds checks can be elided
                for (int i = 0; i < guidArray.Length; i++)
                {
                    AmqpBitConverter.WriteUuid(buffer, guidArray[i]);
                }
            }
            else
            {
                IReadOnlyList<Guid> listValue = (IReadOnlyList<Guid>)value;
                for (int i = 0; i < listValue.Count; i++)
                {
                    AmqpBitConverter.WriteUuid(buffer, listValue[i]);
                }
            }
        }

        public override Guid[] DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode)
        {
            Guid[] array = new Guid[count];
            for (int i = 0; i < count; ++i)
            {
                array[i] = AmqpBitConverter.ReadUuid(buffer);
            }
            return array;
        }
    }
}
