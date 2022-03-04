// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
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
        /// <returns>A task for the async operation.</returns>
        public Task OpenAsync()
        {
            return this.OpenAsync(CancellationToken.None);
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="timeout">A timeout for the open operation.</param>
        /// <returns>A task for the async operation.</returns>
        public Task OpenAsync(TimeSpan timeout)
        {
            return this.OpenAsync(new CancellationTokenSource(timeout).Token);
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A task for the async operation.</returns>
        public Task OpenAsync(CancellationToken cancellationToken)
        {
            return this.GetOrCreateAsync(cancellationToken);
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>A task for the async operation.</returns>
        public Task CloseAsync()
        {
            return this.CloseAsync(CancellationToken.None);
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A task for the async operation.</returns>
        public Task CloseAsync(CancellationToken cancellationToken)
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
                if (thisTaskCompletionSource != null && thisTaskCompletionSource.Task.Status == TaskStatus.RanToCompletion && this.TryRemove())
                {
                    OnSafeClose(thisTaskCompletionSource.Task.Result);
                }
            }
        }

        /// <summary>
        /// Gets or creates TValue object.
        /// </summary>
        /// <param name="timeout">A timeout for the operation.</param>
        /// <returns>A task for the async operation.</returns>
        public Task<TValue> GetOrCreateAsync(TimeSpan timeout)
        {
            return this.GetOrCreateAsync(timeout, CancellationToken.None);
        }

        /// <summary>
        /// Gets or creates TValue object.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A task for the async operation.</returns>
        public Task<TValue> GetOrCreateAsync(CancellationToken cancellationToken)
        {
            return this.GetOrCreateAsync(AmqpConstants.DefaultTimeout, cancellationToken);
        }

        /// <summary>
        /// Gets or creates TValue object (for internal use only).
        /// </summary>
        /// <param name="timeout">A timeout for the operation.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns></returns>
        public async Task<TValue> GetOrCreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            while (timeoutHelper.RemainingTime() > TimeSpan.Zero)
            {
                cancellationToken.ThrowIfCancellationRequested();
                if (this.disposed)
                {
                    throw new OperationCanceledException();
                }

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

                tcs = new TaskCompletionSource<TValue>(TaskCreationOptions.RunContinuationsAsynchronously);
                if (this.TrySet(tcs))
                {
                    try
                    {
                        TValue value = await this.OnCreateAsync(timeoutHelper.RemainingTime(), cancellationToken).ConfigureAwait(false);
                        tcs.SetResult(value);

                        if (this.disposed && this.TryRemove())
                        {
                            OnSafeClose(value);
                        }
                    }
                    catch (Exception ex) when (!Fx.IsFatal(ex))
                    {
                        this.TryRemove();
                        tcs.SetException(ex);
                    }

                    return await tcs.Task.ConfigureAwait(false);
                }
            }

            throw new TimeoutException(AmqpResources.GetString(AmqpResources.AmqpTimeout, timeout, typeof(TValue).Name));
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
        /// <param name="timeout">A timeout for the operation.</param>
        /// <returns>A task for the async operation.</returns>
        protected virtual Task<TValue> OnCreateAsync(TimeSpan timeout) => throw new NotImplementedException();

        /// <summary>
        /// Creates the singleton.
        /// </summary>
        /// <param name="timeout">A timeout for the operation.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns></returns>
        protected virtual Task<TValue> OnCreateAsync(TimeSpan timeout, CancellationToken cancellationToken) => OnCreateAsync(timeout);

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
