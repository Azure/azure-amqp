// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.Serialization;

    [Serializable]
    class CallbackException : FatalException
    {
        public CallbackException()
        {
        }

        public CallbackException(string message, Exception innerException)
            : base(message, innerException)
        {
            // This cannot throw something like ArgumentException because that would be worse than
            // throwing the callback exception that was requested.
            Fx.Assert(innerException != null, "CallbackException requires an inner exception.");
            Fx.Assert(!Fx.IsFatal(innerException), "CallbackException can't be used to wrap fatal exceptions.");
        }

#if NET10_0_OR_GREATER
        [Obsolete(DiagnosticId = "SYSLIB0051")] // Formatter-based serialization is retained for backward compatibility with .NET Framework BinaryFormatter.
#endif
        protected CallbackException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
    }
}
