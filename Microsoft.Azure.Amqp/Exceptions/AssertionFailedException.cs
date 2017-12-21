// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.Serialization;

#if NET45 || MONOANDROID
    [Serializable]
#endif
    class AssertionFailedException : Exception
    {
        public AssertionFailedException(string description)
            : base(CommonResources.GetString(CommonResources.ShipAssertExceptionMessage, description))
        {
        }

#if NET45 || MONOANDROID
        protected AssertionFailedException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}
