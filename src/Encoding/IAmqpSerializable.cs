// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    public interface IAmqpSerializable
    {
        int EncodeSize
        {
            get;
        }

        void Encode(ByteBuffer buffer);

        void Decode(ByteBuffer buffer);
    }
}
