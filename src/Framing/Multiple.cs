// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Represends one or multiple object of T.
    /// </summary>
    /// <remarks>Multiple is not an AMQP data type. It is an encoding property.
    /// When there is only one T object, it is encoded as a single T object.
    /// When there are multiple T objects, it is encoded as an array of T.
    /// Examples are <see cref="Open.OfferedCapabilities"/> and <see cref="Source.Capabilities"/></remarks>
    public sealed class Multiple<T> : List<T>
    {
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
            : base(value)
        {
        }

        internal static int GetEncodeSize(Multiple<T> multiple)
        {
            if (multiple == null)
            {
                return FixedWidth.NullEncoded;
            }
            else if (multiple.Count == 1)
            {
                return AmqpEncoding.GetObjectEncodeSize(multiple[0]);
            }
            else
            {
                return ArrayEncoding.GetEncodeSize<T>(multiple);
            }
        }

        internal static void Encode(Multiple<T> multiple, ByteBuffer buffer)
        {
            if (multiple == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else if (multiple.Count == 1)
            {
                AmqpEncoding.EncodeObject(multiple[0], buffer);
            }
            else
            {
                ArrayEncoding.Encode<T>(multiple, buffer);
            }
        }

        internal static Multiple<T> Decode(ByteBuffer buffer)
        {
            object value = AmqpEncoding.DecodeObject(buffer);
            if (value == null)
            {
                return null;
            }
            else if (value is T)
            {
                Multiple<T> multiple = new Multiple<T>();
                multiple.Add((T)value);
                return multiple;
            }
            else if (value.GetType().IsArray)
            {
                Multiple<T> multiple = new Multiple<T>((T[])value);
                return multiple;
            }
            else
            {
                throw new AmqpException(AmqpErrorCode.InvalidField, null);
            }
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

            foreach (T t1 in multiple1)
            {
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
            foreach (object item in this)
            {
                if (!firstItem)
                {
                    sb.Append(',');
                }

                sb.Append(item.ToString());
                firstItem = false;
            }

            sb.Append(']');
            return sb.ToString();
        }
    }
}
