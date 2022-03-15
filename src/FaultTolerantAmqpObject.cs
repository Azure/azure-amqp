// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Provides a singleton of an AmqpObject. Recreates the object when it is closed.
    /// </summary>
    public sealed class FaultTolerantAmqpObject<T> : Singleton<T> where T : AmqpObject
    {
        readonly Func<TimeSpan, Task<T>> createObjectOld;
        readonly Func<CancellationToken, Task<T>> createObjectAsync;
        readonly Func<TimeSpan, CancellationToken, Task<T>> createObjectInternal;
        readonly Action<T> closeObject;

        FaultTolerantAmqpObject(Action<T> closeObject)
        {
            this.closeObject = closeObject;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="createObjectAsync">The function to create the AmqpObject.</param>
        /// <param name="closeObject">The action to close the AmqpObject.</param>
        public FaultTolerantAmqpObject(Func<TimeSpan, Task<T>> createObjectAsync, Action<T> closeObject)
            : this(closeObject)
        {
            this.createObjectOld = createObjectAsync;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="createObjectAsync">The function to create the AmqpObject.</param>
        /// <param name="closeObject">The action to close the AmqpObject.</param>
        public FaultTolerantAmqpObject(Func<CancellationToken, Task<T>> createObjectAsync, Action<T> closeObject)
            : this(closeObject)
        {
            this.createObjectAsync = createObjectAsync;
        }

        internal FaultTolerantAmqpObject(Func<TimeSpan, CancellationToken, Task<T>> createObjectAsync, Action<T> closeObject)
            : this(closeObject)
        {
            this.createObjectInternal = createObjectAsync;
        }

        /// <summary>
        /// Gets the AmqpObject if it is in open state.
        /// </summary>
        /// <param name="openedAmqpObject">The AmqpObject. Null if it is not created or already closed.</param>
        /// <returns>true if the current object is in open state.</returns>
        public bool TryGetOpenedObject(out T openedAmqpObject)
        {
            T obj = this.Value;
            if (obj != null && obj.State == AmqpObjectState.Opened)
            {
                openedAmqpObject = obj;
                return true;
            }

            openedAmqpObject = null;
            return false;
        }

        /// <inheritdoc cref="Singleton{TValue}"/>
        protected override bool IsValid(T value)
        {
            return value.State == AmqpObjectState.Opened;
        }

        /// <inheritdoc cref="Singleton{TValue}"/>
        protected override async Task<T> OnCreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            T amqpObject;
            if (this.createObjectAsync != null)
            {
                amqpObject = await this.createObjectAsync(cancellationToken).ConfigureAwait(false);
            }
            else if (this.createObjectOld != null)
            {
                amqpObject = await this.createObjectOld(timeout).ConfigureAwait(false);
            }
            else
            {
                amqpObject = await this.createObjectInternal(timeout, cancellationToken).ConfigureAwait(false);
            }

            amqpObject.SafeAddClosed(OnObjectClosed);
            return amqpObject;
        }

        /// <inheritdoc cref="Singleton{TValue}"/>
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
