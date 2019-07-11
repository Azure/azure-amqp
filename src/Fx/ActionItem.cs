// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Threading;

    static class ActionItem
    {
        public static void Schedule(WaitCallback callback, object state)
        {
            Fx.Assert(callback != null, "A null callback was passed for Schedule!");
            ThreadPool.QueueUserWorkItem(callback, state);
        }
    }
}
