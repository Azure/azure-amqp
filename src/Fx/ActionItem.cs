// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    static class ActionItem
    {
        public static void Schedule<TState>(Action<TState> callback, TState state)
        {
            Fx.Assert(callback != null, "A null callback was passed for Schedule!");

            _ = Task.Factory.StartNew(static state =>
                {
                    var (callback, callbackState) = (Tuple<Action<TState>, TState>) state;
                    try
                    {
                        callback(callbackState);
                    }
                    catch (Exception e)
                    {
                        // to make sure the application crashes on unhandled exceptions
                        ThreadPool.QueueUserWorkItem(static state => throw ((Exception) state), e);
                    }
                }, Tuple.Create(callback, state), CancellationToken.None,
                TaskCreationOptions.DenyChildAttach, TaskScheduler.Default);
        }
    }
}