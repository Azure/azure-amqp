// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.ComponentModel;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// This API supports the Azure infrastructure and is not intended to be used directly from your code.
    /// </summary>
    public sealed class FaultTolerantAmqpObject<T> : Singleton<T> where T : AmqpObject
    {
        readonly Func<TimeSpan, Task<T>> createObjectAsync;
        readonly Action<T> closeObject;
        readonly EventHandler onObjectClosed;

        public FaultTolerantAmqpObject(Func<TimeSpan, Task<T>> createObjectAsync, Action<T> closeObject)
        {
            this.createObjectAsync = createObjectAsync;
            this.closeObject = closeObject;
            this.onObjectClosed = new EventHandler(this.OnObjectClosed);
        }

        public bool TryGetOpenedObject(out T openedAmqpObject)
        {
            var taskCompletionSource = this.TaskCompletionSource;
            if (taskCompletionSource != null && taskCompletionSource.Task.Status == TaskStatus.RanToCompletion)
            {
                openedAmqpObject = taskCompletionSource.Task.Result;
                if (openedAmqpObject == null || openedAmqpObject.State != AmqpObjectState.Opened)
                {
                    openedAmqpObject = null;
                }
            }
            else
            {
                openedAmqpObject = null;
            }

            return openedAmqpObject != null;
        }

        protected override bool IsValid(T value)
        {
            return value.State == AmqpObjectState.Opened;
        }

        protected override async Task<T> OnCreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            T amqpObject = await this.createObjectAsync(timeout).ConfigureAwait(false);
            amqpObject.SafeAddClosed(OnObjectClosed);
            return amqpObject;
        }

        // Deprecated, but needs to stay available to avoid
        // breaking changes. The attribute removes it from
        // any code completion listings and doc generation.
        [EditorBrowsable(EditorBrowsableState.Never)]
        protected override Task<T> OnCreateAsync(TimeSpan timeout) => OnCreateAsync(timeout, CancellationToken.None);

        protected override void OnSafeClose(T value)
        {
            this.closeObject(value);
        }

        void OnObjectClosed(object sender, EventArgs e)
        {
            T instance = (T)sender;
            this.closeObject(instance);
            this.Invalidate(instance);
        }
    }
}
