// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections;

    abstract class PrimitiveEncoding : EncodingBase
    {
        protected PrimitiveEncoding(FormatCode formatCode)
            : base(formatCode)
        {
        }

        public abstract int GetArrayEncodeSize(IList value);

        public abstract void EncodeArray(IList value, ByteBuffer buffer);
        public abstract Array DecodeArray(ByteBuffer buffer, int count, FormatCode formatCode);
    }
}
