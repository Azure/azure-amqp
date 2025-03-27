// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.IO;
    using System.Runtime.ExceptionServices;

    static class ExceptionDispatcher
    {
        public static void Throw(Exception exception)
        {
            ExceptionDispatchInfo exceptionDispatchInfo = ExceptionDispatchInfo.Capture(exception);
            exceptionDispatchInfo.Throw();
        }

        public static void Rethrow(this Exception exception)
        {
            Throw(exception);
        }

        public static Exception ToIOException(this Exception exception)
        {
            if (exception is ObjectDisposedException)
            {
                return new IOException("Transport is closed", exception);
            }

            return exception;
        }
    }
}
