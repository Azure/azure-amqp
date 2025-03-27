// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Encoding;
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
    //            in OnRecieveOpen/CloseCommand. Example: initiator.
    //      Sync Open/Close: after Open/CloseInternal, the
    //            state is Opened/End. Everything is completed
    //            synchronously. Example: acceptor.
    //      SafeClose = Close + Exception handling
    //
    // =================================================
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
        IList<AmqpSymbol> mutualCapabilities;

        public event EventHandler<OpenEventArgs> Opening;
        public event EventHandler Opened;
        public event EventHandler Closed;

        protected AmqpObject(string type)
            : this(type, SequenceNumber.Increment(ref AmqpObject.nextId))
        {
        }

        protected AmqpObject(string type, SequenceNumber identifier)
        {
            this.identifier = identifier;
            this.name = type + this.identifier;
        }

        public SequenceNumber Identifier
        {
            get { return this.identifier; }
        }

        public AmqpObjectState State
        {
            get;
            protected set;
        }

        public Exception TerminalException
        {
            get;
            protected set;
        }

        [Obsolete("Use connection or link settings operation timeout.")]
        public TimeSpan DefaultOpenTimeout
        {
            get;
            protected set;
        }

        [Obsolete("Use connection or link settings operation timeout.")]
        public TimeSpan DefaultCloseTimeout
        {
            get;
            protected set;
        }

        public IList<AmqpSymbol> MutualCapabilities
        {
            get
            {
                return this.mutualCapabilities ?? new List<AmqpSymbol>();
            }
        }

        protected object ThisLock
        {
            get { return this.thisLock; }
        }

        internal bool CloseCalled => this.closeCalled;

        internal virtual TimeSpan OperationTimeout
        {
            get { return AmqpConstants.DefaultTimeout; }
        }

        public void Open()
        {
            this.Open(this.OperationTimeout);
        }

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
                this.OpenInternal();
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

        public Task OpenAsync(TimeSpan timeout)
        {
            return Task.Factory.FromAsync(this.BeginOpen, this.EndOpen, timeout, null);
        }

        public Task OpenAsync(CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (t, k, c, s) => ((AmqpObject)s).BeginOpen(t, k, c, s),
                r => ((AmqpObject)r.AsyncState).EndOpen(r),
                this.OperationTimeout,
                cancellationToken,
                this);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (t, k, c, s) => ((AmqpObject) s).BeginClose(t, k, c, s),
                r => ((AmqpObject) r.AsyncState).EndClose(r),
                this.OperationTimeout,
                cancellationToken,
                this);
        }

        public IAsyncResult BeginOpen(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginOpen(timeout, CancellationToken.None, callback, state);
        }

        public void EndOpen(IAsyncResult result)
        {
            OpenAsyncResult.End(result);
            AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, nameof(EndOpen));
        }

        public void Close()
        {
            this.Close(this.OperationTimeout);
        }

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
                this.CloseInternal();
                this.NotifyClosed();
            }
            else
            {
                this.OnClose(timeout);
            }
        }

        public Task CloseAsync(TimeSpan timeout)
        {
            return Task.Factory.FromAsync(this.BeginClose, this.EndClose, timeout, null);
        }

        public IAsyncResult BeginClose(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginClose(timeout, CancellationToken.None, callback, state);
        }

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
        /// Move to End state without closing
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
                AmqpTrace.Provider.AmqpAbortThrowingException(exception.ToStringSlim());
                throw;
            }
            finally
            {
                this.NotifyClosed();
            }
        }

        /// <summary>
        /// Close with exception handling
        /// </summary>
        public void SafeClose()
        {
            this.SafeClose(null);
        }

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
                AmqpTrace.Provider.AmqpLogError(this, nameof(SafeClose), exp.ToString());

                this.Abort();
            }
        }

        public override string ToString()
        {
            return this.name;
        }

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

        public bool IsClosing()
        {
            AmqpObjectState state = this.State;
            return state == AmqpObjectState.CloseSent ||
                state == AmqpObjectState.CloseReceived ||
                state == AmqpObjectState.ClosePipe ||
                state == AmqpObjectState.End ||
                state == AmqpObjectState.Faulted;
        }

        protected virtual void OnOpen(TimeSpan timeout)
        {
            OpenAsyncResult.End(new OpenAsyncResult(this, timeout, CancellationToken.None, null, null));
        }

        protected virtual void OnClose(TimeSpan timeout)
        {
            CloseAsyncResult.End(new CloseAsyncResult(this, timeout, CancellationToken.None, null, null));
        }

        protected abstract bool OpenInternal();
        
        protected abstract bool CloseInternal();

        protected abstract void AbortInternal();

        protected void FindMutualCapabilites(Multiple<AmqpSymbol> desired, Multiple<AmqpSymbol> offered)
        {
            this.mutualCapabilities = Multiple<AmqpSymbol>.Intersect(desired, offered);
        }

        protected void NotifyOpening(Performative command)
        {
            EventHandler<OpenEventArgs> opening = this.Opening;
            if (opening != null)
            {
                opening(this, new OpenEventArgs(command));
            }
        }

        protected void CompleteOpen(bool syncComplete, Exception exception)
        {
            OpenAsyncResult openResult = Interlocked.Exchange(ref this.pendingOpen, null);
            if (openResult != null)
            {
                openResult.Signal(syncComplete, exception);
            }
        }

        protected void CompleteClose(bool syncComplete, Exception exception)
        {
            CloseAsyncResult closeResult = Interlocked.Exchange(ref this.pendingClose, null);
            if (closeResult != null)
            {
                closeResult.Signal(syncComplete, exception);
            }
        }

        protected StateTransition TransitState(string operation, StateTransition[] states)
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

        protected void OnReceiveCloseCommand(string command, Error error)
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

                    this.Close();
                }
            }
            catch (AmqpException exception)
            {
                AmqpTrace.Provider.AmqpLogError(this, command, exception.ToStringSlim());
                this.Abort();
            }
        }

        protected void ThrowIfClosed()
        {
            if (this.closeCalled || this.abortCalled)
            {
                string reason = this.closeCalled ? "closed" : "aborted";
                throw new AmqpException(AmqpErrorCode.IllegalState, $"Object '{this.name}' is {reason}.");
            }
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
                AmqpTrace.Provider.AmqpLogError(thisPtr, "SafeCloseComplete", exception.ToStringSlim());

                thisPtr.Abort();
            }
        }

        internal IAsyncResult BeginOpen(TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
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
            return new OpenAsyncResult(this, timeout, cancellationToken, callback, state);
        }

        IAsyncResult BeginClose(TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
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
                return new CloseAsyncResult(this, timeout, cancellationToken, callback, state);
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

            protected AmqpObjectAsyncResult(AmqpObject amqpObject, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object asyncState)
                : base(timeout, cancellationToken, callback, asyncState)
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
                bool shouldComplete = false;
                Exception completeException = null;

                try
                {
                    shouldComplete = this.OnStart();
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    AmqpTrace.Provider.AmqpLogError(this.amqpObject, "OnStart", exception.ToStringSlim());

                    shouldComplete = true;
                    completeException = exception;
                }

                if (shouldComplete)
                {
                    amqpObject.pendingOpen = null;
                    amqpObject.pendingClose = null;
                    this.Signal(true, completeException);
                }

                this.StartTracking();
            }

            protected abstract bool OnStart();

            protected abstract void UpdateState(Exception exception);
        }

        sealed class OpenAsyncResult : AmqpObjectAsyncResult
        {
            public OpenAsyncResult(AmqpObject amqpObject, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object asyncState)
                : base(amqpObject, timeout, cancellationToken, callback, asyncState)
            {
                amqpObject.pendingOpen = this;
                this.Start();
            }

            public static void End(IAsyncResult result)
            {
                OpenAsyncResult thisPtr = AsyncResult.End<OpenAsyncResult>(result);
                thisPtr.Target.NotifyOpened();
            }

            public override void Cancel(bool isSynchronous)
            {
                this.Target.CompleteOpen(isSynchronous, new TaskCanceledException());
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
            }
        }

        sealed class CloseAsyncResult : AmqpObjectAsyncResult
        {
            public CloseAsyncResult(AmqpObject amqpObject, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object asyncState)
                : base(amqpObject, timeout, cancellationToken, callback, asyncState)
            {
                amqpObject.pendingClose = this;
                this.Start();
            }

            public static void End(IAsyncResult result)
            {
                AsyncResult.End<CloseAsyncResult>(result);
            }

            public override void Cancel(bool isSynchronous)
            {
                this.Target.CompleteClose(isSynchronous, new TaskCanceledException());
            }

            protected override bool OnStart()
            {
                return this.Target.CloseInternal();
            }

            protected override void UpdateState(Exception exception)
            {
                this.Target.State = AmqpObjectState.End;
                // we want the closed event handlers to be run on this thread
                this.Target.NotifyClosed();
            }
        }
    }
}
