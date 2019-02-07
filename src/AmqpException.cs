// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// The exception represents an AMQP error.
    /// </summary>
    [Serializable]
    public sealed class AmqpException : Exception
    {
        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="error">The AMQP error.</param>
        public AmqpException(Error error)
            : base(error.Description ?? AmqpResources.GetString(AmqpResources.AmqpErrorOccurred, error.Condition.Value))
        {
            this.Error = error;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="condition">The error condition.</param>
        /// <param name="description">The error description.</param>
        public AmqpException(AmqpSymbol condition, string description)
            : this(new Error() { Condition = condition, Description = description })
        {
        }

        AmqpException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            this.Error = (Error)info.GetValue("Error", typeof(Error));
        }

        /// <summary>
        /// Gets the <see cref="Error"/> of the exception.
        /// </summary>
        public Error Error
        {
            get;
            private set;
        }

        /// <summary>
        /// Creates an exception from an error.
        /// </summary>
        /// <param name="error">The AMQP error.</param>
        /// <returns></returns>
        public static AmqpException FromError(Error error)
        {
            if (error == null || error.Condition.Value == null)
            {
                return null;
            }

            return new AmqpException(error);
        }

        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);

            info.AddValue("Error", this.Error);
        }
    }
}