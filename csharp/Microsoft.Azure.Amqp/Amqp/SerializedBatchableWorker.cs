//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Work is serialized but all pending work is batched
    /// </summary>
    sealed class SerializedBatchableWorker<T>
    {
        readonly IWorkDelegate<IList<T>> workHandler;
        readonly List<T> pendingList;
        bool working;
        bool closeIssued;

        public SerializedBatchableWorker(IWorkDelegate<IList<T>> workHandler)
        {
            this.workHandler = workHandler;
            this.pendingList = new List<T>();
        }

        object SyncRoot
        {
            get { return this.pendingList; }
        }

        public void DoWork(T work)
        {
            if (this.AddPendingWork(work, null))
            {
                this.DoPendingWork();
            }
        }

        public void DoWork(IList<T> workList)
        {
            if (this.AddPendingWork(default(T), workList))
            {
                this.DoPendingWork();
            }
        }

        public void ContinueWork()
        {
            this.DoPendingWork();
        }

        public void IssueClose()
        {
            bool completed;
            lock (this.SyncRoot)
            {
                this.closeIssued = true;
                completed = !this.working;
            }

            if (completed)
            {
                this.workHandler.Invoke(null);
            }
        }

        bool AddPendingWork(T work, IList<T> workList)
        {
            bool doPendingWork = false;
            lock (this.SyncRoot)
            {
                if (this.closeIssued)
                {
                    return false;
                }

                if (workList != null)
                {
                    this.pendingList.AddRange(workList);
                }
                else
                {
                    this.pendingList.Add(work);
                }

                if (!this.working)
                {
                    doPendingWork = true;
                    this.working = true;
                }
            }

            return doPendingWork;
        }

        void DoPendingWork()
        {
            while (true)
            {
                T[] workingList = null;

                lock (this.SyncRoot)
                {
                    if (this.pendingList.Count == 0)
                    {
                        this.working = false;
                        if (!this.closeIssued)
                        {
                            return;
                        }
                    }
                    else
                    {
                        workingList = this.pendingList.ToArray();
                        this.pendingList.Clear();
                    }
                }

                if (!this.workHandler.Invoke(workingList) || workingList == null)
                {
                    return;
                }
            }
        }
    }
}
