// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;

    /// <summary>
    /// Provides a singleton of an AmqpObject. Recreates the object when it is closed.
    /// </summary>
    public sealed class FaultTolerantAmqpObject<T> : Singleton<T> where T : AmqpObject
    {
        readonly Func<TimeSpan, Task<T>> createObjectAsync;
        readonly Action<T> closeObject;
        readonly EventHandler onObjectClosed;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="createObjectAsync">The function to create the AmqpObject.</param>
        /// <param name="closeObject">The action to close the AmqpObject.</param>
        public FaultTolerantAmqpObject(Func<TimeSpan, Task<T>> createObjectAsync, Action<T> closeObject)
        {
            this.createObjectAsync = createObjectAsync;
            this.closeObject = closeObject;
            this.onObjectClosed = new EventHandler(this.OnObjectClosed);
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

        /// <summary>
        /// Determines if the singleton is in valid state.
        /// </summary>
        /// <param name="value">The AmqpObject.</param>
        /// <returns>true if it is open.</returns>
        protected override bool IsValid(T value)
        {
            return value.State == AmqpObjectState.Opened;
        }

        /// <summary>
        /// Creates the singleton AmqpObject.
        /// </summary>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task for the operation.</returns>
        protected override async Task<T> OnCreateAsync(TimeSpan timeout)
        {
            T amqpObject = await this.createObjectAsync(timeout).ConfigureAwait(false);
            amqpObject.SafeAddClosed(OnObjectClosed);
            return amqpObject;
        }

        /// <summary>
        /// Closes the singleton object.
        /// </summary>
        /// <param name="value"></param>
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
