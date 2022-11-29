// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Represents one or multiple object of T.
    /// </summary>
    /// <remarks>Multiple is not an AMQP data type. It is an encoding property.
    /// When there is only one T object, it is encoded as a single T object.
    /// When there are multiple T objects, it is encoded as an array of T.
    /// Examples are <see cref="Open.OfferedCapabilities"/> and <see cref="Source.Capabilities"/></remarks>
    public sealed class Multiple<T> : List<T>
    {
        static readonly EncodingBase<T> encoding = AmqpEncoding.GetEncoding<T>();

        /// <summary>
        /// Initializes a Multiple object.
        /// </summary>
        public Multiple()
        {
        }

        /// <summary>
        /// Initializes a Multiple object with a list of objects.
        /// </summary>
        /// <param name="value">The list of objects to add to the Multiple object.</param>
        public Multiple(IList<T> value)
            : base(value.Count)
        {
            for (int i = 0; i < value.Count; i++)
            {
                this.Add(value[i]);
            }
        }

        Multiple(int capacity)
            : base(capacity)
        {
        }

        internal static int GetEncodeSize(Multiple<T> multiple)
        {
            if (multiple == null)
            {
                return FixedWidth.NullEncoded;
            }

            if (multiple.Count == 1)
            {
                return encoding.GetSize(multiple[0]);
            }

            // encoded as array32
            int size = ArrayEncoding.PrefixSize;
            for (int i = 0; i < multiple.Count; i++)
            {
                size += encoding.GetSize(multiple[i], i);
            }

            return size;
        }

        internal static void Encode(Multiple<T> multiple, ByteBuffer buffer)
        {
            if (multiple == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else if (multiple.Count == 1)
            {
                encoding.Write(multiple[0], buffer);
            }
            else
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Array32);
                var tracker = SizeTracker.Track(buffer);
                AmqpBitConverter.WriteUInt(buffer, FixedWidth.Int);  // size
                AmqpBitConverter.WriteInt(buffer, multiple.Count);   // count
                AmqpBitConverter.WriteUByte(buffer, encoding.FormatCode);
                for (int i = 0; i < multiple.Count; i++)
                {
                    encoding.Write(multiple[i], buffer, i);
                }

                tracker.CommitExclusive(0);
            }
        }

        internal static Multiple<T> Decode(ByteBuffer buffer)
        {
            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode == FormatCode.Null)
            {
                return null;
            }

            if (formatCode != FormatCode.Array8 && formatCode != FormatCode.Array32)
            {
                T item = encoding.Read(buffer, formatCode);
                return new Multiple<T>(1) { item };
            }

            AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Array8, FormatCode.Array32, out int size, out int count);
            Multiple<T> multiple = new Multiple<T>(count);
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            for (int i = 0; i < count; i++)
            {
                T item = encoding.Read(buffer, formatCode);
                multiple.Add(item);
            }

            return multiple;
        }

        /// <summary>
        /// Returns a new Multiple object that contains items in both Multiple objects.
        /// </summary>
        /// <param name="multiple1">The first Multiple object.</param>
        /// <param name="multiple2">The second Multiple object.</param>
        /// <returns>A Multiple object that contains items in both Multiple objects.</returns>
        public static IList<T> Intersect(Multiple<T> multiple1, Multiple<T> multiple2)
        {
            List<T> list = new List<T>();
            if (multiple1 == null || multiple2 == null)
            {
                return list;
            }

            for (var index = 0; index < multiple1.Count; index++)
            {
                T t1 = multiple1[index];
                if (multiple2.Contains(t1))
                {
                    list.Add(t1);
                }
            }

            return list;
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("[");
            bool firstItem = true;
            foreach (var item in this)
            {
                if (!firstItem)
                {
                    sb.Append(',');
                }

                sb.Append(item);
                firstItem = false;
            }

            sb.Append(']');
            return sb.ToString();
        }
    }
}
