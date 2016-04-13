// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    abstract class EncodingBase
    {
        FormatCode formatCode;

        protected EncodingBase(FormatCode formatCode)
        {
            this.formatCode = formatCode;
        }

        public FormatCode FormatCode 
        {
            get { return this.formatCode; }
        }

        public abstract int GetObjectEncodeSize(object value, bool arrayEncoding);

        public abstract void EncodeObject(object value, bool arrayEncoding, ByteBuffer buffer);

        public abstract object DecodeObject(ByteBuffer buffer, FormatCode formatCode);

        public static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected)
        {
            if (formatCode != expected)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        public static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected1, FormatCode expected2)
        {
            if (formatCode != expected1 && formatCode != expected2)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        public static void VerifyFormatCode(FormatCode formatCode, int offset, FormatCode expected1, FormatCode expected2, FormatCode expected3)
        {
            if (formatCode != expected1 && formatCode != expected2 && formatCode != expected3)
            {
                ThrowInvalidFormatCodeException(formatCode, offset);
            }
        }

        static void ThrowInvalidFormatCodeException(FormatCode formatCode, int offset)
        {
            throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, offset));
        }
    }
}
