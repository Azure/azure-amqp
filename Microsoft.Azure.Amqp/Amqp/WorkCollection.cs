// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;

    interface IWork<TOutcome>
    {
        void Start();

        void Done(bool completedSynchronously, TOutcome outcome);

        void Cancel(bool completedSynchronously, Exception exception);
    }

    sealed class WorkCollection<TKey, TWork, TOutcome> where TWork : class, IWork<TOutcome>
    {
        readonly ConcurrentDictionary<TKey, TWork> pendingWork;
        volatile bool closed;

        public WorkCollection()
            : this(null)
        {
        }

        public WorkCollection(IEqualityComparer<TKey> comparer)
        {
            if (comparer == null)
            {
                this.pendingWork = new ConcurrentDictionary<TKey, TWork>();
            }
            else
            {
                this.pendingWork = new ConcurrentDictionary<TKey, TWork>(comparer);
            }
        }

        public void StartWork(TKey key, TWork work)
        {
            if (!this.pendingWork.TryAdd(key, work))
            {
                throw new InvalidOperationException("A pending operation with the same identifier already exists.");
            }

            if (this.closed)
            {
                this.RemoveWork(key, work);
                work.Cancel(true, new OperationCanceledException("The operation is canceled because the owner is already closed."));
                return;
            }

            try
            {
                work.Start();
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                this.RemoveWork(key, work);
                work.Cancel(true, exception);
            }
        }

        public void CompleteWork(TKey key, bool syncComplete, TOutcome outcome)
        {
            TWork work;
            if (this.pendingWork.TryRemove(key, out work))
            {
                if (syncComplete)
                {
                    work.Done(true, outcome);
                }
                else
                {
                    // Schedule the completion so we do not block the I/O thread
                    ActionItem.Schedule(
                        o => { var state = (Tuple<TWork, TOutcome>)o; state.Item1.Done(false, state.Item2); },
                        new Tuple<TWork, TOutcome>(work, outcome));
                }
            }
        }

        // Returns true if the key is found and removed.
        public bool RemoveWork(TKey key, TWork work)
        {
            if (this.pendingWork.TryRemove(key, out TWork temp))
            {
                if (!object.ReferenceEquals(work, temp))
                {
                    // The same key is used for a different work.
                    // Have to fail the victim to avoid a stuck operation.
                    work.Cancel(false, new OperationCanceledException("The operation is canceled because another one started with the same identifier."));
                }

                return true;
            }

            return false;
        }

        public void Abort()
        {
            this.closed = true;
            ActionItem.Schedule(o =>
                {
                    var thisPtr = (WorkCollection<TKey, TWork, TOutcome>)o;
                    List<TKey> keys = new List<TKey>(thisPtr.pendingWork.Keys);
                    foreach (TKey key in keys)
                    {
                        TWork work;
                        if (thisPtr.pendingWork.TryRemove(key, out work))
                        {
                            work.Cancel(false, new OperationCanceledException("The operation is canceled because the owner is being closed."));
                        }
                    }
                },
                this);
        }
    }
}
