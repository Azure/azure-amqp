// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;

    static class ActionItem
    {
        public static void Schedule<TState>(Action<TState> callback, TState state)
        {
            Fx.Assert(callback != null, "A null callback was passed for Schedule!");

            _ = Task.Factory.FromAsync(
                (stateParameter, c, callbackState) => ((Action<TState>)callbackState).BeginInvoke(stateParameter, c, callbackState),
                r => ((Action<TState>)r.AsyncState).EndInvoke(r), state,
                callback);
        }
    }
}
