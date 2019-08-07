// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Provides a singleton for a type.
    /// </summary>
    /// <typeparam name="TValue">The value type.</typeparam>
    public abstract class Singleton<TValue> : IDisposable where TValue : class
    {
        readonly object syncLock;

        TaskCompletionSource<TValue> taskCompletionSource;
        volatile bool disposed;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Singleton()
        {
            this.syncLock = new object();
        }

        // Test verification only
        internal TValue Value
        {
            get
            {
                var thisTaskCompletionSource = this.taskCompletionSource;
                return thisTaskCompletionSource != null && thisTaskCompletionSource.Task.Status == TaskStatus.RanToCompletion ? thisTaskCompletionSource.Task.Result : null;
            }
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task for the async operation.</returns>
        public Task OpenAsync(TimeSpan timeout)
        {
            return this.GetOrCreateAsync(timeout);
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>A task for the async operation.</returns>
        public Task CloseAsync()
        {
            this.Dispose();

            return Task.CompletedTask;
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        public void Close()
        {
            this.Dispose();
        }

        /// <summary>
        /// Disposes the object.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Releases unmanaged resources and optionally releases managed resources.
        /// </summary>
        /// <param name="disposing">
        /// true to release both managed and unmanaged resources;
        /// false to release only unmanaged resources.
        /// </param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing && !this.disposed)
            {
                this.disposed = true;
                var thisTaskCompletionSource = this.taskCompletionSource;
                if (thisTaskCompletionSource != null && thisTaskCompletionSource.Task.Status == TaskStatus.RanToCompletion)
                {
                    OnSafeClose(thisTaskCompletionSource.Task.Result);
                }
            }
        }

        /// <summary>
        /// Gets or creates TValue object.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task for the async operation.</returns>
        public async Task<TValue> GetOrCreateAsync(TimeSpan timeout)
        {
            var timeoutHelper = new TimeoutHelper(timeout);

            while (!this.disposed && timeoutHelper.RemainingTime() > TimeSpan.Zero)
            {
                TaskCompletionSource<TValue> tcs;

                if (this.TryGet(out tcs))
                {
                    TValue current = await tcs.Task.ConfigureAwait(false);
                    if (this.IsValid(current))
                    {
                        return current;
                    }

                    this.Invalidate(current);
                }

                tcs = new TaskCompletionSource<TValue>();
                if (this.TrySet(tcs))
                {
                    try
                    {
                        TValue value = await this.OnCreateAsync(timeout).ConfigureAwait(false);
                        tcs.SetResult(value);
                    }
                    catch (Exception ex) when (!Fx.IsFatal(ex))
                    {
                        this.TryRemove();
                        tcs.SetException(ex);
                    }

                    return await tcs.Task;
                }
            }

            if (this.disposed)
            {
                throw new ObjectDisposedException(this.GetType().Name);
            }

            throw new TimeoutException(string.Format(CultureInfo.InvariantCulture, "Creation of {0} did not complete in {1} milliseconds.", typeof(TValue).Name, timeout.TotalMilliseconds));
        }

        /// <summary>
        /// Marks the singleton as invalid.
        /// </summary>
        /// <param name="instance"></param>
        protected void Invalidate(TValue instance)
        {
            lock (this.syncLock)
            {
                if (this.taskCompletionSource != null &&
                    this.taskCompletionSource.Task.Status == TaskStatus.RanToCompletion &&
                    this.taskCompletionSource.Task.Result == instance)
                {
                    Volatile.Write<TaskCompletionSource<TValue>>(ref this.taskCompletionSource, null);
                }
            }
        }

        /// <summary>
        /// Determines if the instance is valid.
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        protected virtual bool IsValid(TValue value)
        {
            return true;
        }

        /// <summary>
        /// Creates the singleton.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task for the async operation.</returns>
        protected abstract Task<TValue> OnCreateAsync(TimeSpan timeout);

        /// <summary>
        /// Closes the singleton.
        /// </summary>
        /// <param name="value">The singleton.</param>
        protected abstract void OnSafeClose(TValue value);

        bool TryGet(out TaskCompletionSource<TValue> tcs)
        {
            tcs = Volatile.Read<TaskCompletionSource<TValue>>(ref this.taskCompletionSource);
            return tcs != null;
        }

        bool TrySet(TaskCompletionSource<TValue> tcs)
        {
            lock (this.syncLock)
            {
                if (this.taskCompletionSource == null)
                {
                    Volatile.Write<TaskCompletionSource<TValue>>(ref this.taskCompletionSource, tcs);
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        bool TryRemove()
        {
            lock (this.syncLock)
            {
                if (this.taskCompletionSource != null)
                {
                    Volatile.Write<TaskCompletionSource<TValue>>(ref this.taskCompletionSource, null);
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
    }
}
