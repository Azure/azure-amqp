// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Threading;

    class ActionItem
    {
        public static void Schedule(WaitCallback callback, object state)
        {
            Fx.Assert(callback != null, "A null callback was passed for Schedule!");
#if WINDOWS_UWP
            var t = Windows.System.Threading.ThreadPool.RunAsync((workitem) =>
            {
                callback(state);
            });
#else
            ThreadPool.QueueUserWorkItem(callback, state);
#endif
        }
    }
}

