// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;

    // An AsyncResult that completes as soon as it is instantiated.
    [Serializable]
    class CompletedAsyncResult : AsyncResult
    {
        public CompletedAsyncResult(AsyncCallback callback, object state)
            : base(callback, state)
        {
            Complete(true);
        }

        public CompletedAsyncResult(Exception exception, AsyncCallback callback, object state)
            : base(callback, state)
        {
            Complete(true, exception);
        }

        public static void End(IAsyncResult result)
        {
            AsyncResult.End<CompletedAsyncResult>(result);
        }
    }

    [Serializable]
    class CompletedAsyncResult<T> : AsyncResult
    {
        T data;

        public CompletedAsyncResult(T data, AsyncCallback callback, object state)
            : base(callback, state)
        {
            this.data = data;
            Complete(true);
        }

        public static T End(IAsyncResult result)
        {
            CompletedAsyncResult<T> completedResult = AsyncResult.End<CompletedAsyncResult<T>>(result);
            return completedResult.data;
        }
    }
}
