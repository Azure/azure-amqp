// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Threading;
    using System.Threading.Tasks;

    // ---------------------------------------------------------------------------------
    // Base class for async results that compose other async operations.
    //
    // The goal of this class is to make it easy to write async code
    // that composes other async code with arbitrary logic.  Here is
    // an example:
    //
    //         yield return this.CallAsync(
    //             (thisPtr, t, c, s) => thisPtr.channel.BeginOpen(t, c, s),
    //             (thisPtr, r) => thisPtr.channel.EndOpen(r),
    //             ExceptionPolicy.Transfer
    //         );
    //
    //         foreach (Message m in this.GetMessages())
    //         {
    //             if (this.ShouldSend(m))
    //             {
    //                 yield return this.CallAsync(
    //                     (thisPtr, t, c, s) => thisPtr.channel.BeginSend(t, c, s),
    //                     (thisPtr, r) => thisPtr.channel.EndSend(r),
    //                     ExceptionPolicy.Transfer
    //                 );
    //             }
    //         }
    //
    //         yield return this.CallAsync(
    //             (thisPtr, t, c, s) => thisPtr.channel.BeginClose(t, c, s),
    //             (thisPtr, r) => thisPtr.channel.EndClose(r),
    //             ExceptionPolicy.Transfer
    //         );
    //
    // The idiom of (thisPtr, t, c, s) represents:
    //     (XXXAsyncResult thisPtr, TimeSpan timeout, AsyncCallback callback, object state)
    //
    // The idiom of (thisPtr, r) represents:
    //     (XXXAsyncResult thisPtr, IAsyncResult result)
    //
    // Because these patterns arise so frequently with
    // IteratorAsyncResult, best practice is to use the single letter
    // parameters to maximize the chances the lambda will fit on a
    // single line.
    //
    // This sort of code forms the body of the GetAsyncSteps method.  This
    // method returns an IEnumerable<IAsyncStep>, and by using "yield return",
    // the implementation releases the thread exactly at the async points in
    // the flow.  The Begin and End lambdas are called by the base class, and
    // control resumes just after the yield statement.  The C# iterator syntax
    // handles capturing all the local state of the method.
    //
    // The third parameter is a "ExceptionPolicy". 
    // ExceptionPolicy.Transfer is used if you want the async result to be completed with
    // the exception when the step throws. ExceptionPolicy.Continue is used if you want to ignore
    // the excpetion and continue next steps. The last exception ignored can be accessed with
    // LastAsyncException property.
    // ---------------------------------------------------------------------------------
    // 
    // The TIteratorAsyncResult here is type of the derived class itself that you're implementing:
    // Ex) 
    //    class OpenAsyncResult : IteratorAsyncResult<OpenAsyncResult>
    //    {
    //        ...
    //    }
    //
    // This form permits the iterator to be passed as a parameter to CallAsync delegates, so that instance members 
    // can be referenced without reference to this or local variables, which would result in allocating additional memory to pass the state.
    [DebuggerStepThrough]
    abstract class IteratorAsyncResult<TIteratorAsyncResult> : AsyncResult<TIteratorAsyncResult>
        where TIteratorAsyncResult : IteratorAsyncResult<TIteratorAsyncResult>
    {
        static readonly Action<AsyncResult, Exception> onFinally = IteratorAsyncResult<TIteratorAsyncResult>.Finally;

        static AsyncCompletion stepCallbackDelegate;

        // DON'T make TimeoutHelper readonly field.
        // It is very unfortunate design but TimeoutHelper is a struct (value type) that is mutating. 
        // Declarating it as readonly has side impact that it prevents TimeoutHelper mutating itself causing RemainingTime() method
        // returning the original timeout value everytime.
        TimeoutHelper timeoutHelper;
        volatile bool everCompletedAsynchronously;
        IEnumerator<AsyncStep> steps;
        Exception lastAsyncStepException;

        protected IteratorAsyncResult(TimeSpan timeout, AsyncCallback callback, object state)
            : base(callback, state)
        {
            this.timeoutHelper = new TimeoutHelper(timeout, true);
            this.OnCompleting += IteratorAsyncResult<TIteratorAsyncResult>.onFinally;
        }

        protected delegate IAsyncResult BeginCall(TIteratorAsyncResult thisPtr, TimeSpan timeout, AsyncCallback callback, object state);

        protected delegate void EndCall(TIteratorAsyncResult thisPtr, IAsyncResult ar);

        protected delegate void Call(TIteratorAsyncResult thisPtr, TimeSpan timeout);

        private enum CurrentThreadType
        {
            Synchronous,
            StartingThread,
            Callback
        }

        protected Exception LastAsyncStepException
        {
            get { return this.lastAsyncStepException; }
            set { this.lastAsyncStepException = value; }
        }

        public TimeSpan OriginalTimeout
        {
            get
            {
                return this.timeoutHelper.OriginalTimeout;
            }
        }

        private static AsyncCompletion StepCallbackDelegate
        {
            get
            {
                // The race here is intentional and harmless.
                if (stepCallbackDelegate == null)
                {
                    stepCallbackDelegate = new AsyncCompletion(StepCallback);
                }

                return stepCallbackDelegate;
            }
        }

        // This is typically called at the end of the derived AsyncResult
        // constructor, to start the async operation.
        public IAsyncResult Start()
        {
            Debug.Assert(this.steps == null, "IteratorAsyncResult.Start called twice");
            try
            {
                this.steps = this.GetAsyncSteps();

                this.EnumerateSteps(CurrentThreadType.StartingThread);
            }
            catch (Exception e) when (!Fx.IsFatal(e))
            {
                this.Complete(e);
            }

            return this;
        }

        public TIteratorAsyncResult RunSynchronously()
        {
            Debug.Assert(this.steps == null, "IteratorAsyncResult.RunSynchronously or .Start called twice");
            try
            {
                this.steps = this.GetAsyncSteps();
                this.EnumerateSteps(CurrentThreadType.Synchronous);
            }
            catch (Exception e) when (!Fx.IsFatal(e))
            {
                this.Complete(e);
            }

            return End<TIteratorAsyncResult>(this);
        }

        // Utility method to be called from GetAsyncSteps.  To create an implementation
        // of IAsyncCatch, use the CatchAndTransfer or CatchAndContinue methods.
        protected AsyncStep CallAsync(BeginCall beginCall, EndCall endCall, Call call, ExceptionPolicy policy)
        {
            return new AsyncStep(beginCall, endCall, call, policy);
        }

        protected AsyncStep CallAsync(BeginCall beginCall, EndCall endCall, ExceptionPolicy policy)
        {
            return new AsyncStep(beginCall, endCall, null, policy);
        }

        protected AsyncStep CallParallelAsync<TWorkItem>(ICollection<TWorkItem> workItems, BeginCall<TWorkItem> beginCall, EndCall<TWorkItem> endCall, ExceptionPolicy policy)
        {
            return this.CallAsync(
                (thisPtr, t, c, s) => new ParallelAsyncResult<TWorkItem>(thisPtr, workItems, beginCall, endCall, t, c, s),
                (thisPtr, r) => ParallelAsyncResult<TWorkItem>.End(r),
                policy);
        }

        protected AsyncStep CallParallelAsync<TWorkItem>(ICollection<TWorkItem> workItems, BeginCall<TWorkItem> beginCall, EndCall<TWorkItem> endCall, TimeSpan timeout, ExceptionPolicy policy)
        {
            return this.CallAsync(
                (thisPtr, t, c, s) => new ParallelAsyncResult<TWorkItem>(thisPtr, workItems, beginCall, endCall, timeout, c, s),
                (thisPtr, r) => ParallelAsyncResult<TWorkItem>.End(r),
                policy);
        }

        protected AsyncStep CallTask(Func<TIteratorAsyncResult, TimeSpan, Task> taskFunc, ExceptionPolicy policy)
        {
            return this.CallAsync(
                (thisPtr, t, c, s) =>
                {
                    var task = taskFunc(thisPtr, t);
                    if (task.Status == TaskStatus.Created)
                    {
                        // User func might have created a Task without starting.
                        // This can potentially hang threads.
                        task.Start();
                    }

                    return task.ToAsyncResult(c, s);
                },
                (thisPtr, r) => TaskHelpers.EndAsyncResult(r),
                policy);
        }

        protected AsyncStep CallAsyncSleep(TimeSpan amountToSleep)
        {
            return this.CallAsyncSleep(amountToSleep, CancellationToken.None);
        }

        protected AsyncStep CallAsyncSleep(TimeSpan amountToSleep, CancellationToken cancellationToken)
        {
            Fx.Assert(amountToSleep != TimeSpan.MaxValue, "IteratorAsyncResult cannot delay for TimeSpan.MaxValue!");

            return this.CallAsync(
                (thisPtr, t, c, s) => new SleepAsyncResult(amountToSleep, cancellationToken, c, s),
                (thisPtr, r) => SleepAsyncResult.End(r),
                (thisPtr, t) => Task.Delay(amountToSleep).Wait(),
                ExceptionPolicy.Transfer);
        }

        protected AsyncStep CallCompletedAsyncStep()
        {
            return this.CallAsync(
                (thisPtr, t, c, s) => new CompletedAsyncResult(c, s),
                (thisPtr, r) => CompletedAsyncResult.End(r),
                ExceptionPolicy.Transfer);
        }

        protected TimeSpan RemainingTime()
        {
            return this.timeoutHelper.RemainingTime();
        }

        // The derived AsyncResult implements this method as a C# iterator.
        // The implementation should make no blocking calls.  Instead, it
        // runs synchronous code and can "yield return" the result of calling
        // "CallAsync" to cause an async method invocation.
        protected abstract IEnumerator<AsyncStep> GetAsyncSteps();

        protected void Complete(Exception operationException)
        {
            this.Complete(!this.everCompletedAsynchronously, operationException);
        }

        static bool StepCallback(IAsyncResult result)
        {
            var thisPtr = (IteratorAsyncResult<TIteratorAsyncResult>)result.AsyncState;

            bool syncContinue = thisPtr.CheckSyncContinue(result);

            if (!syncContinue)
            {
                thisPtr.everCompletedAsynchronously = true;

                try
                {
                    // Don't refactor this into a seperate method. It adds one extra call stack reducing readibility of call stack in trace.
                    thisPtr.steps.Current.EndCall((TIteratorAsyncResult)thisPtr, result);
                }
                catch (Exception e) when (!Fx.IsFatal(e) && thisPtr.HandleException(e))
                {
                }

                thisPtr.EnumerateSteps(CurrentThreadType.Callback);
            }

            return syncContinue;
        }

        static void Finally(AsyncResult result, Exception exception)
        {
            var thisPtr = (IteratorAsyncResult<TIteratorAsyncResult>)result;
            try
            {
                IEnumerator<AsyncStep> steps = thisPtr.steps;
                if (steps != null)
                {
                    steps.Dispose();
                }
            }
            catch (Exception e) when (!Fx.IsFatal(e))
            {
                ////MessagingClientEtwProvider.Provider.EventWriteExceptionAsWarning(e.ToStringSlim());
                if (exception == null)
                {
                    throw;
                }
            }
        }

        bool MoveNextStep()
        {
            return this.steps.MoveNext();
        }

        // This runs async steps until one of them completes asynchronously, or until
        // Begin throws on the Start thread with a policy of PassThrough.
        void EnumerateSteps(CurrentThreadType state)
        {
            while (!this.IsCompleted && this.MoveNextStep())
            {
                this.LastAsyncStepException = null;
                AsyncStep step = this.steps.Current;
                if (step.BeginCall != null)
                {
                    IAsyncResult result = null;

                    if (state == CurrentThreadType.Synchronous && step.HasSynchronous)
                    {
                        if (step.Policy == ExceptionPolicy.Transfer)
                        {
                            step.Call((TIteratorAsyncResult)this, this.timeoutHelper.RemainingTime());
                        }
                        else
                        {
                            try
                            {
                                step.Call((TIteratorAsyncResult)this, this.timeoutHelper.RemainingTime());
                            }
                            catch (Exception e) when (!Fx.IsFatal(e) && this.HandleException(e))
                            {
                            }
                        }
                    }
                    else
                    {
                        if (step.Policy == ExceptionPolicy.Transfer)
                        {
                            // Don't refactor this into a seperate method. It adds one extra call stack reducing readibility of call stack in trace.
                            result = step.BeginCall(
                                (TIteratorAsyncResult)this,
                                this.timeoutHelper.RemainingTime(),
                                this.PrepareAsyncCompletion(IteratorAsyncResult<TIteratorAsyncResult>.StepCallbackDelegate),
                                this);
                        }
                        else
                        {
                            try
                            {
                                // Don't refactor this into a seperate method. It adds one extra call stack reducing readibility of call stack in trace.
                                result = step.BeginCall(
                                    (TIteratorAsyncResult)this,
                                    this.timeoutHelper.RemainingTime(),
                                    this.PrepareAsyncCompletion(IteratorAsyncResult<TIteratorAsyncResult>.StepCallbackDelegate),
                                    this);
                            }
                            catch (Exception e) when (!Fx.IsFatal(e) && this.HandleException(e))
                            {
                            }
                        }
                    }

                    if (result != null)
                    {
                        if (!this.CheckSyncContinue(result))
                        {
                            return;
                        }

                        try
                        {
                            // Don't refactor this into a seperate method. It adds one extra call stack reducing readibility of call stack in trace.
                            this.steps.Current.EndCall((TIteratorAsyncResult)this, result);
                        }
                        catch (Exception e) when (!Fx.IsFatal(e) && this.HandleException(e))
                        {
                        }
                    }
                }
            }

            if (!this.IsCompleted)
            {
                this.Complete(!this.everCompletedAsynchronously);
            }
        }

        // Returns true if a handler matched the Exception, false otherwise.
        bool HandleException(Exception e)
        {
            bool handled;

            this.LastAsyncStepException = e;
            AsyncStep step = this.steps.Current;

            switch (step.Policy)
            {
                case ExceptionPolicy.Continue:
                    handled = true;
                    break;
                case ExceptionPolicy.Transfer:
                    handled = false;
                    if (!this.IsCompleted)
                    {
                        this.Complete(e);
                        handled = true;
                    }
                    break;
                default:
                    handled = false;
                    break;
            }

            return handled;
        }

        [DebuggerStepThrough]
        protected struct AsyncStep
        {
            readonly ExceptionPolicy policy;
            readonly BeginCall beginCall;
            readonly EndCall endCall;
            readonly Call call;

            public static readonly AsyncStep Empty = new AsyncStep();

            public AsyncStep(
                BeginCall beginCall,
                EndCall endCall,
                Call call,
                ExceptionPolicy policy)
            {
                this.policy = policy;
                this.beginCall = beginCall;
                this.endCall = endCall;
                this.call = call;
            }

            public BeginCall BeginCall
            {
                get { return this.beginCall; }
            }

            public EndCall EndCall
            {
                get { return this.endCall; }
            }

            public Call Call
            {
                get { return this.call; }
            }

            public bool HasSynchronous
            {
                get
                {
                    return this.call != null;
                }
            }

            public ExceptionPolicy Policy
            {
                get
                {
                    return this.policy;
                }
            }
        }

        protected enum ExceptionPolicy
        {
            /// <summary>
            /// ExceptionPolicy.Transfer is used if you want the async result to be completed with
            /// the exception when the step throws.
            /// </summary>
            Transfer,

            /// <summary>
            /// ExceptionPolicy.Continue is used if you want to ignore
            /// the exception and continue next steps. The last exception ignored can be accessed with
            /// LastAsyncException property.
            /// </summary>
            Continue
        }

        sealed class SleepAsyncResult : AsyncResult<SleepAsyncResult>
        {
            readonly Timer timer;
            int complete;

            CancellationTokenRegistration cancellationTokenRegistration;

            public SleepAsyncResult(TimeSpan amount, CancellationToken cancellationToken, AsyncCallback callback, object state)
                : base(callback, state)
            {
                this.timer = new Timer(s => OnTimer(s), this, amount, Timeout.InfiniteTimeSpan);

                try
                {
                    this.cancellationTokenRegistration = cancellationToken.Register(s => OnCancellation(s), this);
                }
                catch (ObjectDisposedException)
                {
                    this.HandleCancellation(false);
                }
            }

            public static new void End(IAsyncResult result)
            {
                var thisPtr = AsyncResult<SleepAsyncResult>.End(result);
                try
                {
                    thisPtr.cancellationTokenRegistration.Dispose();
                }
                catch (ObjectDisposedException)
                {
                    // .Net 4.0 can throw this. Note that .Net 4.5 doesn't have this problem.
                    // this ODE can happen because HandleCancellation() can race with OnTimer()
                }
            }

            static void OnTimer(object state)
            {
                SleepAsyncResult thisPtr = (SleepAsyncResult)state;
                if (Interlocked.CompareExchange(ref thisPtr.complete, 1, 0) == 0)
                {
                    thisPtr.Complete(false);
                }
            }

            static void OnCancellation(object state)
            {
                SleepAsyncResult thisPtr = (SleepAsyncResult)state;

                // This callback can be called synchronously or asynchronously.
                // To avoid issues, always schedule Complete() call in a seperate thread.
                thisPtr.HandleCancellation(true);
            }

            void HandleCancellation(bool scheduleComplete)
            {
                if (Interlocked.CompareExchange(ref this.complete, 1, 0) == 0)
                {
                    this.timer.Change(Timeout.Infinite, Timeout.Infinite);

                    if (scheduleComplete)
                    {
                        ActionItem.Schedule(s => ((SleepAsyncResult)s).Complete(false), this);
                    }
                    else
                    {
                        this.Complete(true);
                    }
                }
            }
        }

        protected delegate IAsyncResult BeginCall<TWorkItem>(TIteratorAsyncResult thisPtr, TWorkItem workItem, TimeSpan timeout, AsyncCallback callback, object state);

        protected delegate void EndCall<TWorkItem>(TIteratorAsyncResult thisPtr, TWorkItem workItem, IAsyncResult ar);

        sealed class ParallelAsyncResult<TWorkItem> : AsyncResult<ParallelAsyncResult<TWorkItem>>
        {
            static readonly AsyncCallback completed = new AsyncCallback(OnCompleted);

            readonly TIteratorAsyncResult iteratorAsyncResult;
            readonly ICollection<TWorkItem> workItems;
            readonly EndCall<TWorkItem> endCall;
            long actions;
            Exception firstException;

            public ParallelAsyncResult(TIteratorAsyncResult iteratorAsyncResult, ICollection<TWorkItem> workItems, BeginCall<TWorkItem> beginCall, EndCall<TWorkItem> endCall, TimeSpan timeout, AsyncCallback callback, object state)
                : base(callback, state)
            {
                this.iteratorAsyncResult = iteratorAsyncResult;
                this.workItems = workItems;
                this.endCall = endCall;
                this.actions = this.workItems.Count + 1;

                foreach (TWorkItem source in workItems)
                {
                    try
                    {
                        beginCall(iteratorAsyncResult, source, timeout, completed, new CallbackState(this, source));
                    }
                    catch (Exception e) when (!Fx.IsFatal(e))
                    {
                        TryComplete(e, true);
                    }
                }

                TryComplete(null, true);
            }

            void TryComplete(Exception exception, bool completedSynchronously)
            {
                if (this.firstException == null && exception != null)
                {
                    this.firstException = exception;
                }

                if (Interlocked.Decrement(ref this.actions) == 0)
                {
                    this.Complete(completedSynchronously, this.firstException);
                }
            }

            static void OnCompleted(IAsyncResult ar)
            {
                CallbackState state = (CallbackState)ar.AsyncState;
                ParallelAsyncResult<TWorkItem> thisPtr = state.AsyncResult;

                try
                {
                    thisPtr.endCall(thisPtr.iteratorAsyncResult, state.AsyncData, ar);
                    thisPtr.TryComplete(null, ar.CompletedSynchronously);
                }
                catch (Exception e) when (!Fx.IsFatal(e))
                {
                    thisPtr.TryComplete(e, ar.CompletedSynchronously);
                }
            }

            sealed class CallbackState
            {
                public CallbackState(ParallelAsyncResult<TWorkItem> asyncResult, TWorkItem data)
                {
                    this.AsyncResult = asyncResult;
                    this.AsyncData = data;
                }

                public ParallelAsyncResult<TWorkItem> AsyncResult { get; private set; }

                public TWorkItem AsyncData { get; private set; }
            }
        }
    }
}