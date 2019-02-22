// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    /// <summary>
    /// Allows an object to control its own serialization and deserialization.
    /// </summary>
    public interface IAmqpSerializable
    {
        /// <summary>
        /// Gets the encoded size.
        /// </summary>
        int EncodeSize
        {
            get;
        }

        /// <summary>
        /// Encodes the object and writes the bytes to the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        void Encode(ByteBuffer buffer);

        /// <summary>
        /// Decodes the object from the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        void Decode(ByteBuffer buffer);
    }
}
