// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;
    using System.Threading;
    using Microsoft.Azure.Amqp.Tracing;

    // AsyncResult starts acquired; Complete releases.
    [Fx.Tag.SynchronizationPrimitive(Fx.Tag.BlocksUsing.ManualResetEvent, SupportsAsync = true, ReleaseMethod = "Complete")]
    [DebuggerStepThrough]
    [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
        Justification = "Uses custom scheme for cleanup")]
    abstract class AsyncResult : IAsyncResult
    {
        public const string DisablePrepareForRethrow = "DisablePrepareForRethrow";

        static AsyncCallback asyncCompletionWrapperCallback;
        AsyncCallback callback;
        bool completedSynchronously;
        bool endCalled;
        Exception exception;
        bool isCompleted;
        AsyncCompletion nextAsyncCompletion;
        IAsyncResult deferredTransactionalResult;
        object state;

        [Fx.Tag.SynchronizationObject]
        ManualResetEvent manualResetEvent;

        [Fx.Tag.SynchronizationObject(Blocking = false)]
        object thisLock;

#if DEBUG
        UncompletedAsyncResultMarker marker;
#endif

        protected AsyncResult(AsyncCallback callback, object state)
        {
            this.callback = callback;
            this.state = state;
            this.thisLock = new object();

#if DEBUG
            this.marker = new UncompletedAsyncResultMarker(this);
#endif
        }

        public object AsyncState
        {
            get
            {
                return this.state;
            }
        }

        public WaitHandle AsyncWaitHandle
        {
            get
            {
                if (this.manualResetEvent != null)
                {
                    return this.manualResetEvent;
                }

                lock (this.ThisLock)
                {
                    if (this.manualResetEvent == null)
                    {
                        this.manualResetEvent = new ManualResetEvent(isCompleted);
                    }
                }

                return this.manualResetEvent;
            }
        }

        public bool CompletedSynchronously
        {
            get
            {
                return this.completedSynchronously;
            }
        }

        public bool HasCallback
        {
            get
            {
                return this.callback != null;
            }
        }

        public bool IsCompleted
        {
            get
            {
                return this.isCompleted;
            }
        }

        // used in conjunction with PrepareAsyncCompletion to allow for finally blocks
        protected Action<AsyncResult, Exception> OnCompleting { get; set; }

        // Override this property to provide the ActivityId when completing with exception
        protected internal virtual EventTraceActivity Activity
        {
            get { return null; }
        }

        // Override this property to change the trace level when completing with exception
        protected virtual EventLevel EventLevel
        {
            get { return EventLevel.Verbose; }
        }

        protected object ThisLock
        {
            get
            {
                return this.thisLock;
            }
        }

        // subclasses like TraceAsyncResult can use this to wrap the callback functionality in a scope
        protected Action<AsyncCallback, IAsyncResult> VirtualCallback
        {
            get;
            set;
        }

        protected bool TryComplete(bool didCompleteSynchronously, Exception exception)
        {
            lock (this.ThisLock)
            {
                if (this.isCompleted)
                {
                    return false;
                }

                this.exception = exception;
                this.isCompleted = true;
            }

#if DEBUG
            this.marker.AsyncResult = null;
            this.marker = null;
#endif

            this.completedSynchronously = didCompleteSynchronously;
            if (this.OnCompleting != null)
            {
                // Allow exception replacement, like a catch/throw pattern.
                try
                {
                    this.OnCompleting(this, this.exception);
                }
                catch (Exception e)
                {
                    if (Fx.IsFatal(e))
                    {
                        throw;
                    }
                    this.exception = e;
                }
            }

            if (didCompleteSynchronously)
            {
                // If we completedSynchronously, then there's no chance that the manualResetEvent was created so
                // we don't need to worry about a race
                Fx.Assert(this.manualResetEvent == null, "No ManualResetEvent should be created for a synchronous AsyncResult.");
            }
            else
            {
                lock (this.ThisLock)
                {
                    if (this.manualResetEvent != null)
                    {
                        this.manualResetEvent.Set();
                    }
                }
            }

            if (this.callback != null)
            {
                try
                {
                    if (this.VirtualCallback != null)
                    {
                        this.VirtualCallback(this.callback, this);
                    }
                    else
                    {
                        this.callback(this);
                    }
                }
#pragma warning disable 1634
#pragma warning suppress 56500 // transferring exception to another thread
                catch (Exception e)
                {
                    if (Fx.IsFatal(e))
                    {
                        throw;
                    }

                    throw new CallbackException(CommonResources.AsyncCallbackThrewException, e);
                }
#pragma warning restore 1634
            }

            return true;
        }

        protected bool TryComplete(bool didcompleteSynchronously)
        {
            return this.TryComplete(didcompleteSynchronously, null);
        }

        protected void Complete(bool didCompleteSynchronously)
        {
            this.Complete(didCompleteSynchronously, null);
        }

        protected void Complete(bool didCompleteSynchronously, Exception e)
        {
            if (!this.TryComplete(didCompleteSynchronously, e))
            {
                throw new InvalidOperationException(CommonResources.GetString(CommonResources.AsyncResultCompletedTwice, this.GetType()));
            }
        }

        static void AsyncCompletionWrapperCallback(IAsyncResult result)
        {
            if (result == null)
            {
                throw new InvalidOperationException(CommonResources.InvalidNullAsyncResult);
            }
            if (result.CompletedSynchronously)
            {
                return;
            }

            AsyncResult thisPtr = (AsyncResult)result.AsyncState;
            AsyncCompletion callback = thisPtr.GetNextCompletion();
            if (callback == null)
            {
                ThrowInvalidAsyncResult(result);
            }

            bool completeSelf = false;
            Exception completionException = null;
            try
            {
                completeSelf = callback(result);
            }
            catch (Exception e)
            {
                completeSelf = true;
                completionException = e;
            }

            if (completeSelf)
            {
                thisPtr.Complete(false, completionException);
            }
        }

        protected AsyncCallback PrepareAsyncCompletion(AsyncCompletion callback)
        {
            this.nextAsyncCompletion = callback;
            if (AsyncResult.asyncCompletionWrapperCallback == null)
            {
                AsyncResult.asyncCompletionWrapperCallback = new AsyncCallback(AsyncCompletionWrapperCallback);
            }
            return AsyncResult.asyncCompletionWrapperCallback;
        }

        protected bool CheckSyncContinue(IAsyncResult result)
        {
            AsyncCompletion dummy;
            return TryContinueHelper(result, out dummy);
        }

        protected bool SyncContinue(IAsyncResult result)
        {
            AsyncCompletion callback;
            if (TryContinueHelper(result, out callback))
            {
                return callback(result);
            }
            else
            {
                return false;
            }
        }

        bool TryContinueHelper(IAsyncResult result, out AsyncCompletion callback)
        {
            if (result == null)
            {
                throw new InvalidOperationException(CommonResources.InvalidNullAsyncResult);
            }

            callback = null;

            if (result.CompletedSynchronously)
            {
                // Once we pass the check, we know that we own forward progress, so transactionContext is correct. Verify its state.                
            }
            else if (object.ReferenceEquals(result, this.deferredTransactionalResult))
            {
                this.deferredTransactionalResult = null;
            }
            else
            {
                return false;
            }

            callback = GetNextCompletion();
            if (callback == null)
            {
                ThrowInvalidAsyncResult("Only call Check/SyncContinue once per async operation (once per PrepareAsyncCompletion).");
            }
            return true;
        }

        AsyncCompletion GetNextCompletion()
        {
            AsyncCompletion result = this.nextAsyncCompletion;
            this.nextAsyncCompletion = null;
            return result;
        }

        protected static void ThrowInvalidAsyncResult(IAsyncResult result)
        {
            throw new InvalidOperationException(CommonResources.GetString(CommonResources.InvalidAsyncResultImplementation, result.GetType()));
        }

        protected static void ThrowInvalidAsyncResult(string debugText)
        {
            string message = CommonResources.InvalidAsyncResultImplementationGeneric;
            if (debugText != null)
            {
#if DEBUG
                message += " " + debugText;
#endif
            }
            throw new InvalidOperationException(message);
        }

        [Fx.Tag.Blocking(Conditional = "!asyncResult.isCompleted")]
        protected static TAsyncResult End<TAsyncResult>(IAsyncResult result)
            where TAsyncResult : AsyncResult
        {
            if (result == null)
            {
                throw new ArgumentNullException("result");
            }

            TAsyncResult asyncResult = result as TAsyncResult;

            if (asyncResult == null)
            {
                throw new ArgumentException("result", CommonResources.InvalidAsyncResult);
            }

            if (asyncResult.endCalled)
            {
                throw new InvalidOperationException(CommonResources.AsyncResultAlreadyEnded);
            }

            asyncResult.endCalled = true;

            if (!asyncResult.isCompleted)
            {
                lock (asyncResult.ThisLock)
                {
                    if (!asyncResult.isCompleted && asyncResult.manualResetEvent == null)
                    {
                        asyncResult.manualResetEvent = new ManualResetEvent(asyncResult.isCompleted);
                    }
                }
            }

            if (asyncResult.manualResetEvent != null)
            {
                asyncResult.manualResetEvent.WaitOne();
                asyncResult.manualResetEvent.Dispose();
            }

            if (asyncResult.exception != null)
            {
                ExceptionDispatcher.Throw(asyncResult.exception);
            }

            return asyncResult;
        }

        enum TransactionSignalState
        {
            Ready = 0,
            Prepared,
            Completed,
            Abandoned,
        }

        // can be utilized by subclasses to write core completion code for both the sync and async paths
        // in one location, signalling chainable synchronous completion with the boolean result,
        // and leveraging PrepareAsyncCompletion for conversion to an AsyncCallback.
        // NOTE: requires that "this" is passed in as the state object to the asynchronous sub-call being used with a completion routine.
        protected delegate bool AsyncCompletion(IAsyncResult result);

#if DEBUG
        class UncompletedAsyncResultMarker
        {
            public UncompletedAsyncResultMarker(AsyncResult result)
            {
                AsyncResult = result;
            }

            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode,
                Justification = "Debug-only facility")]
            public AsyncResult AsyncResult { get; set; }
        }
#endif
    }

    // Use this as your base class for AsyncResult and you don't have to define the End method.
    abstract class AsyncResult<TAsyncResult> : AsyncResult
        where TAsyncResult : AsyncResult<TAsyncResult>
    {
        protected AsyncResult(AsyncCallback callback, object state)
            : base(callback, state)
        {
        }

        public static TAsyncResult End(IAsyncResult asyncResult)
        {
            return AsyncResult.End<TAsyncResult>(asyncResult);
        }
    }
}
