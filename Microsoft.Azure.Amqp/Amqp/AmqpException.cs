// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

#if !DNXCORE
    [Serializable]
#endif
    public sealed class AmqpException : Exception
    {
        public AmqpException(Error error)
            : base(error.Description ?? AmqpResources.GetString(AmqpResources.AmqpErrorOccurred, error.Condition.Value))
        {
            this.Error = error;
        }

        public AmqpException(AmqpSymbol condition, string description)
            : this(new Error() { Condition = condition, Description = description })
        {
        }

#if !DNXCORE
        AmqpException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            this.Error = (Error)info.GetValue("Error", typeof(Error));
        }
#endif

        public Error Error
        {
            get;
            private set;
        }

        public static AmqpException FromError(Error error)
        {
            if (error == null || error.Condition.Value == null)
            {
                return null;
            }

            return new AmqpException(error);
        }

#if !DNXCORE
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);

            info.AddValue("Error", this.Error);
        }
#endif
    }
}