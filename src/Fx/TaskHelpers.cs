// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    static class TaskHelpers
    {
        public static readonly Task CompletedTask = Task.FromResult(default(VoidTaskResult));

        /// <summary>
        /// Create a Task based on Begin/End IAsyncResult pattern.
        /// </summary>
        /// <param name="begin"></param>
        /// <param name="end"></param>
        /// <param name="state"> 
        /// This parameter helps reduce allocations by passing state to the Funcs. e.g.:
        ///  await TaskHelpers.CreateTask(
        ///      (c, s) => ((Transaction)s).BeginCommit(c, s),
        ///      (a) => ((Transaction)a.AsyncState).EndCommit(a),
        ///      transaction);
        /// </param>
        public static Task CreateTask(Func<AsyncCallback, object, IAsyncResult> begin, Action<IAsyncResult> end, object state = null)
        {
            Task retval;
            try
            {
                retval = Task.Factory.FromAsync(begin, end, state);
            }
            catch (Exception ex) when (!Fx.IsFatal(ex))
            {
                var completionSource = new TaskCompletionSource<object>(state);
                completionSource.SetException(ex);
                retval = completionSource.Task;
            }

            return retval;
        }

        public static Task<T> CreateTask<T>(Func<AsyncCallback, object, IAsyncResult> begin, Func<IAsyncResult, T> end, object state = null)
        {
            Task<T> retval;
            try
            {
                retval = Task<T>.Factory.FromAsync(begin, end, state);
            }
            catch (Exception ex) when (!Fx.IsFatal(ex))
            {
                var completionSource = new TaskCompletionSource<T>(state);
                completionSource.SetException(ex);
                retval = completionSource.Task;
            }

            return retval;
        }

        public static IAsyncResult ToAsyncResult(this Task task, AsyncCallback callback, object state)
        {
            if (task.AsyncState == state)
            {
                if (callback != null)
                {
                    task.ContinueWith(
                        t => callback(t),
                        TaskContinuationOptions.ExecuteSynchronously);
                }

                return task;
            }

            var tcs = new TaskCompletionSource<object>(state);
            task.ContinueWith(
                t =>
                {
                    if (t.IsFaulted)
                    {
                        tcs.TrySetException(t.Exception.InnerExceptions);
                    }
                    else if (t.IsCanceled)
                    {
                        tcs.TrySetCanceled();
                    }
                    else
                    {
                        tcs.TrySetResult(null);
                    }

                    callback?.Invoke(tcs.Task);
                },
                TaskContinuationOptions.ExecuteSynchronously);

            return tcs.Task;
        }

        public static IAsyncResult ToAsyncResult<TResult>(this Task<TResult> task, AsyncCallback callback, object state)
        {
            if (task.AsyncState == state)
            {
                if (callback != null)
                {
                    task.ContinueWith(
                        t => callback(t),
                        TaskContinuationOptions.ExecuteSynchronously);
                }

                return task;
            }

            var tcs = new TaskCompletionSource<TResult>(state);
            task.ContinueWith(
                t =>
                {
                    if (t.IsFaulted)
                    {
                        tcs.TrySetException(t.Exception.InnerExceptions);
                    }
                    else if (t.IsCanceled)
                    {
                        tcs.TrySetCanceled();
                    }
                    else
                    {
                        tcs.TrySetResult(t.Result);
                    }

                    callback?.Invoke(tcs.Task);
                },
                TaskContinuationOptions.ExecuteSynchronously);

            return tcs.Task;
        }

        public static void EndAsyncResult(IAsyncResult asyncResult)
        {
            Task task = asyncResult as Task;
            if (task == null)
            {
                throw new ArgumentException(CommonResources.InvalidAsyncResult);
            }

            task.GetAwaiter().GetResult();
        }

        public static TResult EndAsyncResult<TResult>(IAsyncResult asyncResult)
        {
            Task<TResult> task = asyncResult as Task<TResult>;
            if (task == null)
            {
                throw new ArgumentException(CommonResources.InvalidAsyncResult);
            }

            return task.GetAwaiter().GetResult();
        }

        public static Task WithTimeout(this Task task, TimeSpan timeout, Func<string> errorMessage)
        {
            return WithTimeout(task, timeout, errorMessage, CancellationToken.None);
        }

        public static async Task WithTimeout(this Task task, TimeSpan timeout, Func<string> errorMessage, CancellationToken token)
        {
            if (timeout == TimeSpan.MaxValue)
            {
                timeout = Timeout.InfiniteTimeSpan;
            }
            else if (timeout.TotalMilliseconds > Int32.MaxValue)
            {
                timeout = TimeSpan.FromMilliseconds(Int32.MaxValue);
            }

            if (task.IsCompleted || (timeout == Timeout.InfiniteTimeSpan && token == CancellationToken.None))
            {
                await task.ConfigureAwait(false);
                return;
            }

            using (var cts = CancellationTokenSource.CreateLinkedTokenSource(token))
            {
                if (task == await Task.WhenAny(task, CreateDelayTask(timeout, cts.Token)).ConfigureAwait(false))
                {
                    cts.Cancel();
                    await task.ConfigureAwait(false);
                    return;
                }
            }

            throw new TimeoutException(errorMessage());
        }

        static async Task CreateDelayTask(TimeSpan timeout, CancellationToken token)
        {
            try
            {
                await Task.Delay(timeout, token).ConfigureAwait(false);
            }
            catch (TaskCanceledException)
            {
                // No need to throw. Caller is responsible for detecting
                // which task completed and throwing appropriate Timeout Exception
            }
        }

        [StructLayout(LayoutKind.Sequential, Size = 1)]
        internal struct VoidTaskResult
        {
        }
    }
}
