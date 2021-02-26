// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    // ============= State Diagram ====================
    //
    //                 .=======.
    //                 | start |
    //                 .=======.
    //             S:Open  |   R:Open           
    //           +---------+-----------+          
    //           |                     |          
    //       .==========.     .==============.
    //       | OpenSent |     | OpenReceived |
    //       .==========.     .==============.
    //           | R:Open              |S:Open    
    //           +---------+-----------+          
    //                     |                   
    //                .========.               
    //                | Opened |               
    //                .========.               
    //             S:Close |  R:Close          
    //           +---------+-----------+          
    //           |                     |          
    //      .===========.    .===============. 
    //      | CloseSent |    | CloseReceived | 
    //      .===========.    .===============. 
    //           | R:Close             |S:Close   
    //           +---------+-----------+          
    //                     |                   
    //                 .=======.               
    //                 |  End  |
    //                 .=======.
    //        
    //      OpenInternal  = Init1 + S:Open + Init2 if Opened
    //      CloseInternal = Cleanup1 + S:Close + Cleanup2 if End
    //
    //      Async Open/Close: after Open/CloseInternal, the
    //            state is Open/CloseSent. It is waiting for
    //            for the Open/Close command. It is completed
    //            in OnReceiveOpen/CloseCommand. Example: initiator.
    //      Sync Open/Close: after Open/CloseInternal, the
    //            state is Opened/End. Everything is completed
    //            synchronously. Example: acceptor.
    //      SafeClose = Close + Exception handling
    //
    // =================================================

    /// <summary>
    /// Base class of AMQP objects.
    /// </summary>
    public abstract class AmqpObject
    {
        readonly static AsyncCallback onSafeCloseComplete = OnSafeCloseComplete;
        static int nextId = -1;

        readonly SequenceNumber identifier;
        readonly object thisLock = new object();
        string name;
        OpenAsyncResult pendingOpen;
        CloseAsyncResult pendingClose;
        bool openCalled;
        bool closeCalled;
        bool abortCalled;
        bool closedHandlerInvoked;

        /// <summary>
        /// Invoked when an Open is received.
        /// </summary>
        public event EventHandler<OpenEventArgs> Opening;

        /// <summary>
        /// Invoked when the object enters the <see cref="AmqpObjectState.Opened"/> state.
        /// </summary>
        public event EventHandler Opened;

        /// <summary>
        /// Invoked when the object enters the <see cref="AmqpObjectState.End"/> state.
        /// </summary>
        public event EventHandler Closed;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="type">Used as a prefix in the name for tracking/debugging purposes.</param>
        protected AmqpObject(string type)
            : this(type, SequenceNumber.Increment(ref AmqpObject.nextId))
        {
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="type">Used as a prefix in the name for tracking/debugging purposes.</param>
        /// <param name="identifier">Identifier of the object.</param>
        protected AmqpObject(string type, SequenceNumber identifier)
        {
            this.identifier = identifier;
            this.name = type + this.identifier;
        }

        /// <summary>
        /// Gets the identifier.
        /// </summary>
        public SequenceNumber Identifier
        {
            get { return this.identifier; }
        }

        /// <summary>
        /// Gets the current state.
        /// </summary>
        public AmqpObjectState State
        {
            get;
            protected set;
        }

        /// <summary>
        /// Gets the terminal exception of the object.
        /// </summary>
        public Exception TerminalException
        {
            get;
            protected set;
        }

        internal object ThisLock
        {
            get { return this.thisLock; }
        }

        internal bool CloseCalled => this.closeCalled;

        /// <summary>
        /// Opens the object with default timeout.
        /// </summary>
        public void Open()
        {
            this.Open(AmqpConstants.DefaultTimeout);
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="timeout">The open timeout.</param>
        public void Open(TimeSpan timeout)
        {
            bool syncOpen = false;
            lock (this.thisLock)
            {
                if (this.openCalled)
                {
                    throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpInvalidReOpenOperation, this, this.State));
                }

                this.openCalled = true;
                if (this.State == AmqpObjectState.OpenReceived)
                {
                    syncOpen = true;
                }
            }

            if (syncOpen)
            {
                bool outcome = this.OpenInternal();
                Fx.Assert(outcome, "OpenInternal should return true for sync open");
                this.NotifyOpened();
            }
            else
            {
                this.OnOpen(timeout);
            }
        }

        internal void SetName(string name)
        {
            this.name = name;
        }

        /// <summary>
        /// Starts a task to open the object with default timeout.
        /// </summary>
        /// <returns>A task.</returns>
        public Task OpenAsync()
        {
            return this.OpenAsync(AmqpConstants.DefaultTimeout);
        }

        /// <summary>
        /// Starts a task to open the object.
        /// </summary>
        /// <param name="timeout">The open timeout.</param>
        /// <returns>A task.</returns>
        public Task OpenAsync(TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                (t, c, s) => ((AmqpObject)s).BeginOpen(t, c, s),
                (a) => ((AmqpObject)a.AsyncState).EndOpen(a),
                timeout,
                this);
        }

        /// <summary>
        /// Begins to open the object.
        /// </summary>
        /// <param name="timeout">The open timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginOpen(TimeSpan timeout, AsyncCallback callback, object state)
        {
            lock (this.thisLock)
            {
                if (this.openCalled)
                {
                    throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpInvalidReOpenOperation, this, this.State));
                }

                this.openCalled = true;
            }

            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, nameof(BeginOpen));
            return new OpenAsyncResult(this, timeout, callback, state);
        }

        /// <summary>
        /// Ends the open operation.
        /// </summary>
        /// <param name="result">The result returned by the begin method.</param>
        public void EndOpen(IAsyncResult result)
        {
            OpenAsyncResult.End(result);
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, nameof(EndOpen));
        }

        /// <summary>
        /// Closes the object with default timeout.
        /// </summary>
        public void Close()
        {
            this.Close(AmqpConstants.DefaultTimeout);
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <param name="timeout">The close timeout.</param>
        public void Close(TimeSpan timeout)
        {
            bool syncClose = false;
            lock (this.ThisLock)
            {
                bool closed = (this.closeCalled ||
                    this.State == AmqpObjectState.End ||
                    this.State == AmqpObjectState.CloseSent);
                if (closed)
                {
                    return;
                }

                this.closeCalled = true;
                if (this.State == AmqpObjectState.CloseReceived)
                {
                    syncClose = true;
                }
            }

            if (syncClose)
            {
                bool outcome = this.CloseInternal();
                Fx.Assert(outcome, "CloseInternal should return true for sync close");
                this.NotifyClosed();
            }
            else
            {
                this.OnClose(timeout);
            }
        }

        /// <summary>
        /// Starts a task to close the object with default timeout.
        /// </summary>
        /// <returns>A task.</returns>
        public Task CloseAsync()
        {
            return this.CloseAsync(AmqpConstants.DefaultTimeout);
        }

        /// <summary>
        /// Starts a task to close the object.
        /// </summary>
        /// <param name="timeout">The close timeout.</param>
        /// <returns>A task.</returns>
        public Task CloseAsync(TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                (t, c, s) => ((AmqpObject)s).BeginClose(t, c, s),
                (a) => ((AmqpObject)a.AsyncState).EndClose(a),
                timeout,
                this);
        }

        /// <summary>
        /// Begins to close the object.
        /// </summary>
        /// <param name="timeout">The close timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The state associated with this operation.</param>
        /// <returns>An IAsyncResult for the operation.</returns>
        public IAsyncResult BeginClose(TimeSpan timeout, AsyncCallback callback, object state)
        {
            bool closed = false;
            lock (this.ThisLock)
            {
                closed = (this.closeCalled ||
                    this.State == AmqpObjectState.End ||
                    this.State == AmqpObjectState.CloseSent);
                this.closeCalled = true;
            }

            if (closed)
            {
                return new CompletedAsyncResult(callback, state);
            }
            else
            {
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, nameof(BeginClose));
                return new CloseAsyncResult(this, timeout, callback, state);
            }
        }

        /// <summary>
        /// Ends the close operation.
        /// </summary>
        /// <param name="result">The result returned by the begin method.</param>
        public void EndClose(IAsyncResult result)
        {
            if (result is CompletedAsyncResult)
            {
                CompletedAsyncResult.End(result);
            }
            else
            {
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, nameof(EndClose));
                CloseAsyncResult.End(result);
            }
        }

        /// <summary>
        /// Moves to End state without closing.
        /// </summary>
        public void Abort()
        {
            lock (this.ThisLock)
            {
                if (this.abortCalled || this.State == AmqpObjectState.End)
                {
                    return;
                }

                this.State = AmqpObjectState.End;
                this.abortCalled = true;
            }

            try
            {
                this.AbortInternal();
            }
            catch (Exception exception)
            {
                // No one is supposed to throw but it someone does, we need to investigate
                AmqpTrace.Provider.AmqpAbortThrowingException(exception);
                throw;
            }
            finally
            {
                this.NotifyClosed();
            }
        }

        /// <summary>
        /// Closes the object with exception handling in the background.
        /// </summary>
        public void SafeClose()
        {
            this.SafeClose(null);
        }

        /// <summary>
        /// Closes the object with exception handling in the background.
        /// </summary>
        /// <param name="exception">The error for closing the object.</param>
        public void SafeClose(Exception exception)
        {
            this.TerminalException = exception;

            lock (this.thisLock)
            {
                if (this.State != AmqpObjectState.OpenReceived &&
                    !this.IsClosing() &&
                    !StateTransition.CanTransite(this.State, StateTransition.SendClose))
                {
                    this.State = AmqpObjectState.Faulted;
                }
            }

            try
            {
                this.BeginClose(AmqpConstants.DefaultTimeout, onSafeCloseComplete, this);
            }
            catch (Exception exp) when (!Fx.IsFatal(exp))
            {
                AmqpTrace.Provider.AmqpLogError(this, nameof(SafeClose), exp);

                this.Abort();
            }
        }

        /// <summary>
        /// Gets a string representation of the object.
        /// </summary>
        /// <returns>A string representation of the object.</returns>
        public override string ToString()
        {
            return this.name;
        }

        /// <summary>
        /// Registers a handler to <see cref="Closed"/> event.
        /// The handler is invoked even when the object is closed.
        /// </summary>
        /// <param name="handler"></param>
        public void SafeAddClosed(EventHandler handler)
        {
            bool invokeHandler = false;
            lock (this.ThisLock)
            {
                if (this.closedHandlerInvoked)
                {
                    invokeHandler = true;
                }
                else
                {
                    this.Closed += handler;
                }
            }

            if (invokeHandler)
            {
                // The object Closed before our adding the event handler
                // ensure the EventHandler gets invoked anyway
                handler(this, EventArgs.Empty);
            }
        }

        /// <summary>
        /// Returns if the object is closing or closed.
        /// </summary>
        /// <returns>true if the object is closing or closed, false otherwise.</returns>
        public bool IsClosing()
        {
            AmqpObjectState state = this.State;
            return state == AmqpObjectState.CloseSent ||
                state == AmqpObjectState.CloseReceived ||
                state == AmqpObjectState.ClosePipe ||
                state == AmqpObjectState.End ||
                state == AmqpObjectState.Faulted;
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <param name="timeout">The timeout.</param>
        /// <remarks>
        /// The default implementation starts the asynchronous open and waits
        /// for it to complete. Override the method to perform different operations.
        /// </remarks>
        protected virtual void OnOpen(TimeSpan timeout)
        {
            OpenAsyncResult.End(new OpenAsyncResult(this, timeout, null, null));
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <param name="timeout">The timeout.</param>
        /// <remarks>
        /// The default implementation starts the asynchronous close and waits
        /// for it to complete. Override the method to perform different operations.
        /// </remarks>
        protected virtual void OnClose(TimeSpan timeout)
        {
            CloseAsyncResult.End(new CloseAsyncResult(this, timeout, null, null));
        }

        /// <summary>
        /// Override the method to perform the actual open.
        /// </summary>
        /// <returns>True if the object is opened; false if it is pending.</returns>
        protected abstract bool OpenInternal();

        /// <summary>
        /// Override the method to perform the actual close.
        /// </summary>
        /// <returns>True if the object is closed; false if it is pending.</returns>
        protected abstract bool CloseInternal();

        /// <summary>
        /// Override the method to perform the actual abort.
        /// </summary>
        protected abstract void AbortInternal();

        internal void NotifyOpening(Performative command)
        {
            EventHandler<OpenEventArgs> opening = this.Opening;
            if (opening != null)
            {
                opening(this, new OpenEventArgs(command));
            }
        }

        internal void CompleteOpen(bool syncComplete, Exception exception)
        {
            OpenAsyncResult openResult = Interlocked.Exchange(ref this.pendingOpen, null);
            if (openResult != null)
            {
                openResult.Signal(syncComplete, exception);
            }
        }

        internal void CompleteClose(bool syncComplete, Exception exception)
        {
            CloseAsyncResult closeResult = Interlocked.Exchange(ref this.pendingClose, null);
            if (closeResult != null)
            {
                closeResult.Signal(syncComplete, exception);
            }
        }

        internal StateTransition TransitState(string operation, StateTransition[] states)
        {
            StateTransition state = null;

            lock (this.ThisLock)
            {
                foreach (StateTransition st in states)
                {
                    if (st.From == this.State)
                    {
                        this.State = st.To;
                        state = st;
                        break;
                    }
                }
            }

            if (state == null)
            {
                throw new AmqpException(AmqpErrorCode.IllegalState, AmqpResources.GetString(AmqpResources.AmqpIllegalOperationState, operation, this.State));
            }

            AmqpTrace.Provider.AmqpStateTransition(this, operation, state.From, state.To);

            return state;
        }

        internal void OnReceiveCloseCommand(string command, Error error)
        {
            Exception remoteException = null;
            if (error != null && this.TerminalException == null)
            {
                // do not throw exception if the local exception is set
                remoteException = new AmqpException(error);
                this.TerminalException = remoteException;
            }

            try
            {
                StateTransition stateTransition = this.TransitState(command, StateTransition.ReceiveClose);
                if (stateTransition.To == AmqpObjectState.End)
                {
                    this.CompleteClose(false, remoteException);
                }
                else
                {
                    if (this.TerminalException != null)
                    {
                        this.CompleteOpen(false, this.TerminalException);
                    }

                    this.HandleCloseCommand();
                }
            }
            catch (AmqpException exception)
            {
                AmqpTrace.Provider.AmqpLogError(this, command, exception);
                this.Abort();
            }
        }

        internal void ThrowIfClosed()
        {
            if (this.closeCalled)
            {
                throw new AmqpException(AmqpErrorCode.IllegalState, $"'{this.name}' is closed");
            }
        }

        /// <summary>
        /// Called when the AmqpObject is about to finish following OnReceiveCloseCommand.
        /// This gives any derived classes a place to do asynchronous work before letting
        /// the close finish.
        /// For example, between Link R:DETACH and S:DETACH, between Session R:END and S:END
        /// or between Connection R:CLOSE and S:CLOSE.
        /// Once any async work is finished the overriding class should continue
        /// by calling base.HandleCloseCommand (or this.Close).
        /// </summary>
        protected virtual void HandleCloseCommand()
        {
            this.Close();
        }

        static void OnSafeCloseComplete(IAsyncResult result)
        {
            AmqpObject thisPtr = (AmqpObject)result.AsyncState;
            try
            {
                thisPtr.EndClose(result);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                AmqpTrace.Provider.AmqpLogError(thisPtr, "SafeCloseComplete", exception);

                thisPtr.Abort();
            }
        }

        void NotifyOpened()
        {
            EventHandler opened = Interlocked.Exchange(ref this.Opened, null);
            if (opened != null)
            {
                opened(this, EventArgs.Empty);
            }
        }

        void NotifyClosed()
        {
            // Make sure no pending async result is leaked
            if (this.pendingOpen != null)
            {
                this.CompleteOpen(false, this.TerminalException ?? new OperationCanceledException(AmqpResources.GetString(AmqpResources.AmqpObjectAborted, this.name)));
            }

            if (this.pendingClose != null)
            {
                this.CompleteClose(false, this.TerminalException ?? new OperationCanceledException(AmqpResources.GetString(AmqpResources.AmqpObjectAborted, this.name)));
            }

            EventHandler closed = null;
            lock (this.thisLock)
            {
                if (!this.closedHandlerInvoked)
                {
                    closed = this.Closed;
                    this.closedHandlerInvoked = true;
                }
            }

            closed?.Invoke(this, EventArgs.Empty);
        }

        abstract class AmqpObjectAsyncResult : TimeoutAsyncResult<AmqpObject>
        {
            readonly AmqpObject amqpObject;

            protected AmqpObjectAsyncResult(AmqpObject amqpObject, TimeSpan timeout, AsyncCallback callback, object asyncState)
                : base(timeout, callback, asyncState)
            {
                this.amqpObject = amqpObject;
            }

            protected override AmqpObject Target
            {
                get { return this.amqpObject; }
            }

            public void Signal(bool syncComplete, Exception exception)
            {
                this.UpdateState(exception);
                this.CompleteSelf(syncComplete, exception);
            }

            protected void Start()
            {
                this.SetTimer();

                bool shouldComplete = false;
                Exception completeException = null;

                try
                {
                    shouldComplete = this.OnStart();
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    AmqpTrace.Provider.AmqpLogError(this.amqpObject, "OnStart", exception);

                    shouldComplete = true;
                    completeException = exception;
                }

                if (shouldComplete)
                {
                    this.Signal(true, completeException);
                }
            }

            protected abstract bool OnStart();

            protected abstract void UpdateState(Exception exception);
        }

        sealed class OpenAsyncResult : AmqpObjectAsyncResult
        {
            public OpenAsyncResult(AmqpObject amqpObject, TimeSpan timeout, AsyncCallback callback, object asyncState)
                : base(amqpObject, timeout, callback, asyncState)
            {
                amqpObject.pendingOpen = this;
                this.Start();
            }

            public static void End(IAsyncResult result)
            {
                OpenAsyncResult thisPtr = AsyncResult.End<OpenAsyncResult>(result);
                thisPtr.Target.NotifyOpened();
            }

            protected override bool OnStart()
            {
                lock (this.Target.thisLock)
                {
                    if ((this.Target.closeCalled || this.Target.abortCalled) && !this.IsCompleted)
                    {
                        throw new OperationCanceledException();
                    }
                }

                return this.Target.OpenInternal();
            }

            protected override void UpdateState(Exception exception)
            {
                if (exception == null)
                {
                    this.Target.State = AmqpObjectState.Opened;
                }

                this.Target.pendingOpen = null;
            }
        }

        sealed class CloseAsyncResult : AmqpObjectAsyncResult
        {
            public CloseAsyncResult(AmqpObject amqpObject, TimeSpan timeout, AsyncCallback callback, object asyncState)
                : base(amqpObject, timeout, callback, asyncState)
            {
                amqpObject.pendingClose = this;
                this.Start();
            }

            public static void End(IAsyncResult result)
            {
                AsyncResult.End<CloseAsyncResult>(result);
            }

            protected override bool OnStart()
            {
                return this.Target.CloseInternal();
            }

            protected override void UpdateState(Exception exception)
            {
                this.Target.State = AmqpObjectState.End;
                this.Target.pendingClose = null;

                // we want the closed event handlers to be run on this thread
                this.Target.NotifyClosed();
            }
        }
    }
}
