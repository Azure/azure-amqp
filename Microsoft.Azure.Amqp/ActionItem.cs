// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Security;
    using System.Threading;

    abstract class ActionItem
    {
#if !DNXCORE
        [Fx.Tag.SecurityNote(Critical = "Stores the security context, used later in binding back into")]
        [SecurityCritical]
        SecurityContext context;
#endif

        bool isScheduled;

        protected ActionItem()
        {
        }

        [Fx.Tag.SecurityNote(Critical = "Calls into critical method ScheduleCallback",
            Safe = "Schedule invoke of the given delegate under the current context")]
        public static void Schedule(WaitCallback callback, object state)
        {
            Fx.Assert(callback != null, "A null callback was passed for Schedule!");

#if !DNXCORE
            if (PartialTrustHelpers.ShouldFlowSecurityContext || WaitCallbackActionItem.ShouldUseActivity)
            {
                new DefaultActionItem(callback, state).Schedule();
            }
            else
#endif
            {
                ScheduleCallback(callback, state);
            }
        }

        [Fx.Tag.SecurityNote(Critical = "Called after applying the user context on the stack or (potentially) " +
            "without any user context on the stack")]
        [SecurityCritical]
        protected abstract void Invoke();

        [Fx.Tag.SecurityNote(Critical = "Access critical field context and critical property " +
            "CallbackHelper.InvokeWithContextCallback, calls into critical method " +
            "PartialTrustHelpers.CaptureSecurityContextNoIdentityFlow, calls into critical method ScheduleCallback; " +
            "since the invoked method and the capturing of the security contex are de-coupled, can't " +
            "be treated as safe")]
        [SecurityCritical]
        protected void Schedule()
        {
            if (isScheduled)
            {
                throw Fx.Exception.AsError(new InvalidOperationException(CommonResources.ActionItemIsAlreadyScheduled));
            }

            this.isScheduled = true;
#if !DNXCORE
            if (PartialTrustHelpers.ShouldFlowSecurityContext)
            {
                this.context = PartialTrustHelpers.CaptureSecurityContextNoIdentityFlow();
            }
            if (this.context != null)
            {
                ScheduleCallback(CallbackHelper.InvokeWithContextCallback);
            }
            else
#endif // !DNXCORE
            {
                ScheduleCallback(CallbackHelper.InvokeWithoutContextCallback);
            }
        }

#if !DNXCORE
        [Fx.Tag.SecurityNote(Critical = "Access critical field context and critical property " +
            "CallbackHelper.InvokeWithContextCallback, calls into critical method ScheduleCallback; " +
            "since nothing is known about the given context, can't be treated as safe")]
        [SecurityCritical]
        protected void ScheduleWithContext(SecurityContext contextToSchedule)
        {
            if (contextToSchedule == null)
            {
                throw Fx.Exception.ArgumentNull("context");
            }
            if (isScheduled)
            {
                throw Fx.Exception.AsError(new InvalidOperationException(CommonResources.ActionItemIsAlreadyScheduled));
            }

            this.isScheduled = true;
            this.context = contextToSchedule.CreateCopy();
            ScheduleCallback(CallbackHelper.InvokeWithContextCallback);
        }
#endif // !DNXCORE

        [Fx.Tag.SecurityNote(Critical = "Access critical property CallbackHelper.InvokeWithoutContextCallback, " +
            "Calls into critical method ScheduleCallback; not bound to a security context")]
        [SecurityCritical]
        protected void ScheduleWithoutContext()
        {
            if (isScheduled)
            {
                throw Fx.Exception.AsError(new InvalidOperationException(CommonResources.ActionItemIsAlreadyScheduled));
            }

            this.isScheduled = true;
            ScheduleCallback(CallbackHelper.InvokeWithoutContextCallback);
        }

        [Fx.Tag.SecurityNote(Critical = "Calls into critical methods IOThreadScheduler.ScheduleCallbackNoFlow, " +
            "IOThreadScheduler.ScheduleCallbackLowPriNoFlow")]
        [SecurityCritical]
        static void ScheduleCallback(WaitCallback callback, object state)
        {
            Fx.Assert(callback != null, "Cannot schedule a null callback");
            ThreadPool.QueueUserWorkItem(callback, state);
        }

#if !DNXCORE
        [Fx.Tag.SecurityNote(Critical = "Extract the security context stored and reset the critical field")]
        [SecurityCritical]
        SecurityContext ExtractContext()
        {
            Fx.Assert(this.context != null, "Cannot bind to a null context; context should have been set by now");
            Fx.Assert(this.isScheduled, "Context is extracted only while the object is scheduled");
            SecurityContext result = this.context;
            this.context = null;
            return result;
        }
#endif // !DNXCORE

        [Fx.Tag.SecurityNote(Critical = "Calls into critical static method ScheduleCallback")]
        [SecurityCritical]
        void ScheduleCallback(WaitCallback callback)
        {
            ScheduleCallback(callback, this);
        }

        [SecurityCritical]
        static class CallbackHelper
        {
#if !DNXCORE
            [Fx.Tag.SecurityNote(Critical = "Stores a delegate to a critical method")]
            static WaitCallback invokeWithContextCallback;
#endif

            [Fx.Tag.SecurityNote(Critical = "Stores a delegate to a critical method")]
            static WaitCallback invokeWithoutContextCallback;

            [Fx.Tag.SecurityNote(Critical = "Stores a delegate to a critical method")]
            static ContextCallback onContextAppliedCallback;

#if !DNXCORE
            [Fx.Tag.SecurityNote(Critical = "Provides access to a critical field; Initialize it with a delegate to a critical method")]
            public static WaitCallback InvokeWithContextCallback
            {
                get
                {
                    if (invokeWithContextCallback == null)
                    {
                        invokeWithContextCallback = InvokeWithContext;
                    }
                    return invokeWithContextCallback;
                }
            }
#endif // !DNXCORE

            [Fx.Tag.SecurityNote(Critical = "Provides access to a critical field; Initialize it with a delegate to a critical method")]
            public static WaitCallback InvokeWithoutContextCallback
            {
                get
                {
                    if (invokeWithoutContextCallback == null)
                    {
                        invokeWithoutContextCallback = InvokeWithoutContext;
                    }
                    return invokeWithoutContextCallback;
                }
            }

            [Fx.Tag.SecurityNote(Critical = "Provides access to a critical field; Initialize it with a delegate to a critical method")]
            public static ContextCallback OnContextAppliedCallback
            {
                get
                {
                    if (onContextAppliedCallback == null)
                    {
                        onContextAppliedCallback = new ContextCallback(OnContextApplied);
                    }
                    return onContextAppliedCallback;
                }
            }

#if !DNXCORE
            [Fx.Tag.SecurityNote(Critical = "Called by the scheduler without any user context on the stack")]
            static void InvokeWithContext(object state)
            {
                SecurityContext context = ((ActionItem)state).ExtractContext();
                SecurityContext.Run(context, OnContextAppliedCallback, state);
            }
#endif // !DNXCORE

            [Fx.Tag.SecurityNote(Critical = "Called by the scheduler without any user context on the stack")]
            static void InvokeWithoutContext(object state)
            {
                ActionItem tempState = (ActionItem)state;
                tempState.Invoke();
                tempState.isScheduled = false;
            }

            [Fx.Tag.SecurityNote(Critical = "Called after applying the user context on the stack")]
            static void OnContextApplied(object o)
            {
                ActionItem tempState = (ActionItem)o;
                tempState.Invoke();
                tempState.isScheduled = false;
            }
        }

        class DefaultActionItem : ActionItem
        {
            [Fx.Tag.SecurityNote(Critical = "Stores a delegate that will be called later, at a particular context")]
            [SecurityCritical]
            WaitCallback callback;
            [Fx.Tag.SecurityNote(Critical = "Stores an object that will be passed to the delegate that will be " +
                "called later, at a particular context")]
            [SecurityCritical]
            object state;

            [Fx.Tag.SecurityNote(Critical = "Access critical fields callback and state",
                Safe = "Doesn't leak information or resources")]
            public DefaultActionItem(WaitCallback callback, object state)
            {
                Fx.Assert(callback != null, "Shouldn't instantiate an object to wrap a null callback");
                this.callback = callback;
                this.state = state;
            }

            [Fx.Tag.SecurityNote(Critical = "Implements a the critical abstract ActionItem.Invoke method, " +
                "Access critical fields callback and state")]
            [SecurityCritical]
            protected override void Invoke()
            {
                this.callback(this.state);
            }
        }
    }
}

