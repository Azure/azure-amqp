// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// This API supports the Azure infrastructure and is not intended to be used directly from your code.
    /// </summary>
    public abstract class Singleton<TValue> : IDisposable where TValue : class
    {
        readonly object syncLock;
        TaskCompletionSource<TValue> taskCompletionSource;
        volatile bool disposed;

        public Singleton()
        {
            this.syncLock = new object();
        }

        protected TaskCompletionSource<TValue> TaskCompletionSource
        {
            get
            {
                return this.taskCompletionSource;
            }
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

        public Task OpenAsync()
        {
            return this.OpenAsync(CancellationToken.None);
        }

        // Deprecated, but needs to stay available to avoid
        // breaking changes. The attribute removes it from
        // any code completion listings and doc generation.
        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task OpenAsync(TimeSpan timeout)
        {
            return this.GetOrCreateAsync(timeout);
        }

        public Task OpenAsync(CancellationToken cancellationToken)
        {
            return this.GetOrCreateAsync(cancellationToken);
        }

        public Task CloseAsync()
        {
            return this.CloseAsync(CancellationToken.None);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            this.Dispose();
            return TaskHelpers.CompletedTask;
        }

        public void Close()
        {
            this.Dispose();
        }

        /// <inheritdoc />
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
                if (thisTaskCompletionSource != null &&
                    thisTaskCompletionSource.Task.Status == TaskStatus.RanToCompletion &&
                    this.TryRemove())
                {
                    this.OnSafeClose(thisTaskCompletionSource.Task.Result);
                }
            }
        }

        public Task<TValue> GetOrCreateAsync(TimeSpan timeout)
        {
            return this.GetOrCreateAsync(timeout, CancellationToken.None);
        }

        public Task<TValue> GetOrCreateAsync(CancellationToken cancellationToken)
        {
            return this.GetOrCreateAsync(TimeSpan.MaxValue, cancellationToken);
        }

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

        protected virtual bool IsValid(TValue value)
        {
            return true;
        }

        // Deprecated, but needs to stay available to avoid
        // breaking changes. The attribute removes it from
        // any code completion listings and doc generation.
        //
        // This method used to be abstract, but can no longer be to
        // to avoid callers bound to the new versions from needing to
        // implement, despite it being deprecated.
        [EditorBrowsable(EditorBrowsableState.Never)]
        protected virtual Task<TValue> OnCreateAsync(TimeSpan timeout) => throw new NotImplementedException();

        // This approach is not ideal, as the cancellation token is silently
        // ignored if this method is not overridden in derived classes.
        //
        // However, it is necessary to avoid breaking changes for callers bound
        // to earlier versions, as they will not have implemented this new method,
        // so it cannot be abstract and must delegate to the legacy implementation.
        protected virtual Task<TValue> OnCreateAsync(TimeSpan timeout, CancellationToken cancellationToken) => OnCreateAsync(timeout);

        protected abstract void OnSafeClose(TValue value);

        internal bool TryGet(out TValue value, Func<TValue, bool> condition)
        {
            var taskCompletionSource = this.TaskCompletionSource;
            if (taskCompletionSource != null && taskCompletionSource.Task.Status == TaskStatus.RanToCompletion)
            {
                value = TaskCompletionSource.Task.Result;
                if (value != null && condition(value))
                {
                    return true;
                }
            }

            value = null;
            return false;
        }

        // Deprecated, but needs to stay available to avoid
        // breaking changes. The attribute removes it from
        // any code completion listings and doc generation.
        [EditorBrowsable(EditorBrowsableState.Never)]
        public async Task<TValue> GetOrCreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
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
#if NETSTANDARD || MONOANDROID || WINDOWS_UWP
                tcs = new TaskCompletionSource<TValue>(TaskCreationOptions.RunContinuationsAsynchronously);
#else
                tcs = new TaskCompletionSource<TValue>();
#endif
                if (this.TrySet(tcs))
                {
                    try
                    {
                        TValue value = await this.OnCreateAsync(timeout, cancellationToken).ConfigureAwait(false);
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

                    return await tcs.Task;
                }
            }

            if (this.disposed)
            {
                throw new ObjectDisposedException(this.GetType().Name);
            }

            throw new TimeoutException(string.Format(CultureInfo.InvariantCulture, "Creation of {0} did not complete in {1} milliseconds.", typeof(TValue).Name, timeout.TotalMilliseconds));
        }

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
