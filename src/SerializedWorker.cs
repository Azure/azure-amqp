// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Collections.Generic;

    /// <summary>
    /// Work is serialized. Each work is retried until it is completed
    /// </summary>
    public sealed class SerializedWorker<T> where T : class
    {
        enum State
        {
            Idle,
            Busy,
            BusyWithContinue,
            WaitingForContinue,
            Aborted
        }

        // the delegate should return true if work is completed
        readonly IWorkDelegate<T> workDelegate;
        readonly LinkedList<T> pendingWorkList;
        State state;

        public SerializedWorker(IWorkDelegate<T> workProcessor)
        {
            this.workDelegate = workProcessor;
            this.state = State.Idle;
            this.pendingWorkList = new LinkedList<T>();
        }

        public int Count
        {
            get
            {
                return this.pendingWorkList.Count;
            }
        }

        object SyncRoot
        {
            get { return this.pendingWorkList; }
        }

        public void DoWork(T work)
        {
            lock (this.SyncRoot)
            {
                if (this.state == State.Aborted)
                {
                    return;
                }
                else if (this.state != State.Idle)
                {
                    // Only do new work in idle state
                    this.pendingWorkList.AddLast(work);
                    return;
                }

                this.state = State.Busy;
            }

            this.DoWorkInternal(work, false);
        }

        public void ContinueWork()
        {
            T work = null;
            lock (this.SyncRoot)
            {
                if (this.state == State.BusyWithContinue || this.state == State.Aborted)
                {
                    return;
                }
                else if (this.state == State.Busy)
                {
                    this.state = State.BusyWithContinue;
                    return;
                }

                // Idle or WaitingForContinue, we should do the work
                if (this.pendingWorkList.First != null)
                {
                    work = this.pendingWorkList.First.Value;
                    this.state = State.Busy;
                }
            }

            if (work != null)
            {
                this.DoWorkInternal(work, true);
            }
        }

        public void Abort()
        {
            lock (this.SyncRoot)
            {
                this.pendingWorkList.Clear();
                this.state = State.Aborted;
            }
        }

        void DoWorkInternal(T work, bool fromList)
        {
            while (work != null)
            {
                if (this.workDelegate.Invoke(work))
                {
                    lock (this.SyncRoot)
                    {
                        work = null;
                        if (this.state != State.Aborted)
                        {
                            if (fromList && this.pendingWorkList.First != null)
                            {
                                this.pendingWorkList.RemoveFirst();
                            }

                            if (this.pendingWorkList.First != null)
                            {
                                work = this.pendingWorkList.First.Value;
                                fromList = true;
                            }

                            if (work == null)
                            {
                                // either there is no work or the worker was aborted
                                this.state = State.Idle;
                                return;
                            }

                            this.state = State.Busy;
                        }
                    }
                }
                else
                {
                    lock (this.SyncRoot)
                    {
                        if (this.state == State.Aborted)
                        {
                            work = null;
                        }
                        else if (this.state == State.BusyWithContinue)
                        {
                            // Continue called right after workFunc returned false
                            this.state = State.Busy;
                        }
                        else
                        {
                            if (!fromList)
                            {
                                // add to the head since later work may be queued already
                                this.pendingWorkList.AddFirst(work);
                            }

                            this.state = State.WaitingForContinue;
                            work = null;
                        }
                    }
                }
            }
        }
    }
}