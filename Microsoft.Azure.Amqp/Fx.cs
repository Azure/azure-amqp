// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Reflection;
    using System.Runtime.CompilerServices;
#if !DNXCORE
    using System.Runtime.ConstrainedExecution;
#endif
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Threading;
    using Microsoft.Win32;

    static class Fx
    {
        // This is only used for EventLog Source therefore matching EventLog source rather than ETL source
        const string DefaultEventSource = "Microsoft.DeviceHub";

#if DEBUG
        const string SBRegistryKey = @"SOFTWARE\Microsoft\DeviceHub\v2.0";
        const string AssertsFailFastName = "AssertsFailFast";
        const string BreakOnExceptionTypesName = "BreakOnExceptionTypes";
        const string FastDebugName = "FastDebug";

        static bool breakOnExceptionTypesRetrieved;
        static Type[] breakOnExceptionTypesCache;
        static bool fastDebugRetrieved;
        static bool fastDebugCache;
#endif

#if UNUSED
        [Fx.Tag.SecurityNote(Critical = "This delegate is called from within a ConstrainedExecutionRegion, must not be settable from PT code")]
        [SecurityCritical]
        static ExceptionHandler asynchronousThreadExceptionHandler;
#endif // UNUSED

        static ExceptionTrace exceptionTrace;

        ////static DiagnosticTrace diagnosticTrace;

        ////[Fx.Tag.SecurityNote(Critical = "Accesses SecurityCritical field EtwProvider",
        ////    Safe = "Doesn't leak info\\resources")]
        ////[SuppressMessage(FxCop.Category.ReliabilityBasic, FxCop.Rule.UseNewGuidHelperRule,
        ////    Justification = "This is a method that creates ETW provider passing Guid Provider ID.")]
        ////static DiagnosticTrace InitializeTracing()
        ////{
        ////    DiagnosticTrace trace = new DiagnosticTrace(DefaultEventSource, DiagnosticTrace.DefaultEtwProviderId);
        ////    return trace;
        ////}

#if UNUSED
        public static ExceptionHandler AsynchronousThreadExceptionHandler
        {
            [Fx.Tag.SecurityNote(Critical = "access critical field", Safe = "ok for get-only access")]
            [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
            get
            {
                return Fx.asynchronousThreadExceptionHandler;
            }

            [Fx.Tag.SecurityNote(Critical = "sets a critical field")]
            [SecurityCritical]
            set
            {
                Fx.asynchronousThreadExceptionHandler = value;
            }
        }
#endif // UNUSED

        public static ExceptionTrace Exception
        {
            get
            {
                if (exceptionTrace == null)
                {
                    //need not be a true singleton. No locking needed here.
                    exceptionTrace = new ExceptionTrace(DefaultEventSource);
                }

                return exceptionTrace;
            }
        }

        ////public static DiagnosticTrace Trace
        ////{
        ////    get
        ////    {
        ////        if (diagnosticTrace == null)
        ////        {
        ////            diagnosticTrace = InitializeTracing();
        ////        }

        ////        return diagnosticTrace;
        ////    }
        ////}

        public static byte[] AllocateByteArray(int size)
        {
#if !DNXCORE
            try
            {
#endif
                // Safe to catch OOM from this as long as the ONLY thing it does is a simple allocation of a primitive type (no method calls).
                return new byte[size];
#if !DNXCORE
            }
            catch (OutOfMemoryException exception)
            {
                // Convert OOM into an exception that can be safely handled by higher layers.
                throw Fx.Exception.AsError(new InsufficientMemoryException(CommonResources.GetString(CommonResources.BufferAllocationFailed, size), exception));
            }
#endif
        }

        // Do not call the parameter "message" or else FxCop thinks it should be localized.
        [Conditional("DEBUG")]
        public static void Assert(bool condition, string description)
        {
            if (!condition)
            {
                Assert(description);
            }
        }

        [Conditional("DEBUG")]
        public static void Assert(string description)
        {
            Debug.Assert(false, description);
        }

        public static void AssertAndThrow(bool condition, string description)
        {
            if (!condition)
            {
                AssertAndThrow(description);
            }
        }

        public static void AssertIsNotNull(object objectMayBeNull, string description)
        {
            if (objectMayBeNull == null)
            {
                Fx.AssertAndThrow(description);
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static Exception AssertAndThrow(string description)
        {
            Fx.Assert(description);
            throw Fx.Exception.AsError(new AssertionFailedException(description));
        }

        public static void AssertAndThrowFatal(bool condition, string description)
        {
            if (!condition)
            {
                AssertAndThrowFatal(description);
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static Exception AssertAndThrowFatal(string description)
        {
            Fx.Assert(description);
            throw Fx.Exception.AsError(new FatalException(description));
        }

        public static void AssertAndFailFastService(bool condition, string description)
        {
            if (!condition)
            {
                AssertAndFailFastService(description);
            }
        }

        // This never returns.  The Exception return type lets you write 'throw AssertAndFailFast()' which tells the compiler/tools that
        // execution stops.
        [Fx.Tag.SecurityNote(Critical = "Calls into critical method Environment.FailFast",
            Safe = "The side affect of the app crashing is actually intended here")]
        [MethodImpl(MethodImplOptions.NoInlining)]
        public static Exception AssertAndFailFastService(string description)
        {
            Fx.Assert(description);
            string failFastMessage = CommonResources.GetString(CommonResources.FailFastMessage, description);

            // The catch is here to force the finally to run, as finallys don't run until the stack walk gets to a catch.  
            // The catch makes sure that the finally will run before the stack-walk leaves the frame, but the code inside is impossible to reach.
            try
            {
                try
                {
                    ////MessagingClientEtwProvider.Provider.EventWriteFailFastOccurred(description);
                    Fx.Exception.TraceFailFast(failFastMessage);

                    // Mark that a FailFast is in progress, so that we can take ourselves out of the NLB if for
                    // any reason we can't kill ourselves quickly.  Wait 15 seconds so this state gets picked up for sure.
                    Fx.FailFastInProgress = true;
                    Thread.Sleep(TimeSpan.FromSeconds(15));
                }
                finally
                {
                    // ########################## NOTE ###########################
                    // Environment.FailFast does not collect crash dumps when used in Azure services. 
                    // Environment.FailFast(failFastMessage);

                    // ################## WORKAROUND #############################
                    // Workaround for the issue above. Throwing an unhandled exception on a separate thread to trigger process crash and crash dump collection
                    // Throwing FatalException since our service does not morph/eat up fatal exceptions
                    // We should find the tracking bug in Azure for this issue, and remove the workaround when fixed by Azure
                    Thread failFastWorkaroundThread = new Thread(delegate()
                    {
                        throw new FatalException(failFastMessage);
                    });

                    failFastWorkaroundThread.Start();
                    failFastWorkaroundThread.Join();
                }
            }
            catch
            {
                throw;
            }

            return null; // we'll never get here since we've just fail-fasted
        }

        internal static bool FailFastInProgress { get; private set; }

        public static bool IsFatal(Exception exception)
        {
            while (exception != null)
            {
                // FYI, CallbackException is-a FatalException
                if (exception is FatalException ||
#if !DNXCORE
                    (exception is OutOfMemoryException && !(exception is InsufficientMemoryException)) ||
                    exception is ThreadAbortException ||
                    exception is AccessViolationException ||
#endif
                    exception is SEHException)
                {
                    return true;
                }

                // These exceptions aren't themselves fatal, but since the CLR uses them to wrap other exceptions,
                // we want to check to see whether they've been used to wrap a fatal exception.  If so, then they
                // count as fatal.
                if (exception is TypeInitializationException ||
                    exception is TargetInvocationException)
                {
                    exception = exception.InnerException;
                }
                else if (exception is AggregateException)
                {
                    // AggregateExceptions have a collection of inner exceptions, which may themselves be other
                    // wrapping exceptions (including nested AggregateExceptions).  Recursively walk this
                    // hierarchy.  The (singular) InnerException is included in the collection.
                    ReadOnlyCollection<Exception> innerExceptions = ((AggregateException)exception).InnerExceptions;
                    foreach (Exception innerException in innerExceptions)
                    {
                        if (IsFatal(innerException))
                        {
                            return true;
                        }
                    }

                    break;
                }
                else if (exception is NullReferenceException)
                {
                    ////MessagingClientEtwProvider.Provider.EventWriteNullReferenceErrorOccurred(exception.ToString());
                    break;
                }
                else
                {
                    break;
                }
            }

            return false;
        }

#if DEBUG
        internal static bool AssertsFailFast
        {
            get
            {
                object value;
                return TryGetDebugSwitch(Fx.AssertsFailFastName, out value) &&
                    typeof(int).IsAssignableFrom(value.GetType()) && ((int)value) != 0;
            }
        }

        internal static Type[] BreakOnExceptionTypes
        {
            get
            {
                if (!Fx.breakOnExceptionTypesRetrieved)
                {
                    object value;
                    if (TryGetDebugSwitch(Fx.BreakOnExceptionTypesName, out value))
                    {
                        string[] typeNames = value as string[];
                        if (typeNames != null && typeNames.Length > 0)
                        {
                            List<Type> types = new List<Type>(typeNames.Length);
                            for (int i = 0; i < typeNames.Length; i++)
                            {
                                types.Add(Type.GetType(typeNames[i], false));
                            }
                            if (types.Count != 0)
                            {
                                Fx.breakOnExceptionTypesCache = types.ToArray();
                            }
                        }
                    }
                    Fx.breakOnExceptionTypesRetrieved = true;
                }
                return Fx.breakOnExceptionTypesCache;
            }
        }

        internal static bool FastDebug
        {
            get
            {
                if (!Fx.fastDebugRetrieved)
                {
                    object value;
                    if (TryGetDebugSwitch(Fx.FastDebugName, out value))
                    {
                        Fx.fastDebugCache = typeof(int).IsAssignableFrom(value.GetType()) && ((int)value) != 0;
                    }

                    Fx.fastDebugRetrieved = true;
                    ////MessagingClientEtwProvider.Provider.EventWriteLogAsWarning(string.Format(CultureInfo.InvariantCulture, "AppDomain({0}).Fx.FastDebug={1}", AppDomain.CurrentDomain.FriendlyName, Fx.fastDebugCache.ToString()));
                }

                return Fx.fastDebugCache;
            }
        }

        static bool TryGetDebugSwitch(string name, out object value)
        {
            value = null;
#if !DNXCORE
            try
            {
                RegistryKey key = Registry.LocalMachine.OpenSubKey(Fx.SBRegistryKey);
                if (key != null)
                {
                    using (key)
                    {
                        value = key.GetValue(name);
                    }
                }
            }
            catch (SecurityException)
            {
                // This debug-only code shouldn't trace.
            }
#endif // !DNXCORE
            return value != null;
        }
#endif // DEBUG

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes,
            Justification = "Don't want to hide the exception which is about to crash the process.")]
        [Fx.Tag.SecurityNote(Miscellaneous = "Must not call into PT code as it is called within a CER.")]
#if !DNXCORE
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
#endif
        static void TraceExceptionNoThrow(Exception exception)
        {
            try
            {
                // This call exits the CER.  However, when still inside a catch, normal ThreadAbort is prevented.
                // Rude ThreadAbort will still be allowed to terminate processing.
                Fx.Exception.TraceUnhandled(exception);
            }
            catch
            {
                // This empty catch is only acceptable because we are a) in a CER and b) processing an exception
                // which is about to crash the process anyway.
            }
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes,
            Justification = "Don't want to hide the exception which is about to crash the process.")]
        [SuppressMessage(FxCop.Category.ReliabilityBasic, FxCop.Rule.IsFatalRule,
            Justification = "Don't want to hide the exception which is about to crash the process.")]
        [Fx.Tag.SecurityNote(Miscellaneous = "Must not call into PT code as it is called within a CER.")]
#if !DNXCORE
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
#endif
        static bool HandleAtThreadBase(Exception exception)
        {
            // This area is too sensitive to do anything but return.
            if (exception == null)
            {
                Fx.Assert("Null exception in HandleAtThreadBase.");
                return false;
            }

            TraceExceptionNoThrow(exception);

#if UNUSED
            try
            {
                ExceptionHandler handler = Fx.AsynchronousThreadExceptionHandler;
                return handler == null ? false : handler.HandleException(exception);
            }
            catch (Exception secondException)
            {
                // Don't let a new exception hide the original exception.
                TraceExceptionNoThrow(secondException);
            }
#endif // UNUSED

            return false;
        }

        public static class Tag
        {
            public enum CacheAttrition
            {
                None,
                ElementOnTimer,

                // A finalizer/WeakReference based cache, where the elements are held by WeakReferences (or hold an
                // inner object by a WeakReference), and the weakly-referenced object has a finalizer which cleans the
                // item from the cache.
                ElementOnGC,

                // A cache that provides a per-element token, delegate, interface, or other piece of context that can
                // be used to remove the element (such as IDisposable).
                ElementOnCallback,

                FullPurgeOnTimer,
                FullPurgeOnEachAccess,
                PartialPurgeOnTimer,
                PartialPurgeOnEachAccess,
            }

            public enum Location
            {
                InProcess,
                OutOfProcess,
                LocalSystem,
                LocalOrRemoteSystem, // as in a file that might live on a share
                RemoteSystem,
            }

            public enum SynchronizationKind
            {
                LockStatement,
                MonitorWait,
                MonitorExplicit,
                InterlockedNoSpin,
                InterlockedWithSpin,

                // Same as LockStatement if the field type is object.
                FromFieldType,
            }

            [Flags]
            public enum BlocksUsing
            {
                MonitorEnter,
                MonitorWait,
                ManualResetEvent,
                AutoResetEvent,
                AsyncResult,
                IAsyncResult,
                PInvoke,
                InputQueue,
                ThreadNeutralSemaphore,
                PrivatePrimitive,
                OtherInternalPrimitive,
                OtherFrameworkPrimitive,
                OtherInterop,
                Other,

                NonBlocking, // For use by non-blocking SynchronizationPrimitives such as IOThreadScheduler
            }

            public static class Strings
            {
                internal const string ExternallyManaged = "externally managed";
                internal const string AppDomain = "AppDomain";
                internal const string DeclaringInstance = "instance of declaring class";
                internal const string Unbounded = "unbounded";
                internal const string Infinite = "infinite";
            }

            [AttributeUsage(AttributeTargets.Field | AttributeTargets.Method | AttributeTargets.Constructor,
                AllowMultiple = true, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class ExternalResourceAttribute : Attribute
            {
                readonly Location location;
                readonly string description;

                public ExternalResourceAttribute(Location location, string description)
                {
                    this.location = location;
                    this.description = description;
                }

                public Location Location
                {
                    get
                    {
                        return this.location;
                    }
                }

                public string Description
                {
                    get
                    {
                        return this.description;
                    }
                }
            }

            [AttributeUsage(AttributeTargets.Field)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class CacheAttribute : Attribute
            {
                readonly Type elementType;
                readonly CacheAttrition cacheAttrition;

                public CacheAttribute(Type elementType, CacheAttrition cacheAttrition)
                {
                    Scope = Strings.DeclaringInstance;
                    SizeLimit = Strings.Unbounded;
                    Timeout = Strings.Infinite;

                    if (elementType == null)
                    {
                        throw Fx.Exception.ArgumentNull("elementType");
                    }

                    this.elementType = elementType;
                    this.cacheAttrition = cacheAttrition;
                }

                public Type ElementType
                {
                    get
                    {
                        return this.elementType;
                    }
                }

                public CacheAttrition CacheAttrition
                {
                    get
                    {
                        return this.cacheAttrition;
                    }
                }

                public string Scope { get; set; }
                public string SizeLimit { get; set; }
                public string Timeout { get; set; }
            }

            [AttributeUsage(AttributeTargets.Field)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class QueueAttribute : Attribute
            {
                readonly Type elementType;

                public QueueAttribute(Type elementType)
                {
                    Scope = Strings.DeclaringInstance;
                    SizeLimit = Strings.Unbounded;

                    if (elementType == null)
                    {
                        throw Fx.Exception.ArgumentNull("elementType");
                    }

                    this.elementType = elementType;
                }

                public Type ElementType
                {
                    get
                    {
                        return this.elementType;
                    }
                }

                public string Scope { get; set; }
                public string SizeLimit { get; set; }
                public bool StaleElementsRemovedImmediately { get; set; }
                public bool EnqueueThrowsIfFull { get; set; }
            }

            // Set on a class when that class uses lock (this) - acts as though it were on a field
            //     private object this;
            [AttributeUsage(AttributeTargets.Field | AttributeTargets.Class, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class SynchronizationObjectAttribute : Attribute
            {
                public SynchronizationObjectAttribute()
                {
                    Blocking = true;
                    Scope = Strings.DeclaringInstance;
                    Kind = SynchronizationKind.FromFieldType;
                }

                public bool Blocking
                {
                    get;
                    set;
                }
                public string Scope
                {
                    get;
                    set;
                }
                public SynchronizationKind Kind
                {
                    get;
                    set;
                }
            }

            [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = true)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class SynchronizationPrimitiveAttribute : Attribute
            {
                readonly BlocksUsing blocksUsing;

                public SynchronizationPrimitiveAttribute(BlocksUsing blocksUsing)
                {
                    this.blocksUsing = blocksUsing;
                }

                public BlocksUsing BlocksUsing
                {
                    get
                    {
                        return this.blocksUsing;
                    }
                }

                public bool SupportsAsync
                {
                    get;
                    set;
                }
                public bool Spins
                {
                    get;
                    set;
                }
                public string ReleaseMethod
                {
                    get;
                    set;
                }
            }

            [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class BlockingAttribute : Attribute
            {
                public BlockingAttribute()
                {
                }

                public string CancelMethod
                {
                    get;
                    set;
                }
                public Type CancelDeclaringType
                {
                    get;
                    set;
                }
                public string Conditional
                {
                    get;
                    set;
                }
            }

            // Sometime a method will call a conditionally-blocking method in such a way that it is guaranteed
            // not to block (i.e. the condition can be Asserted false).  Such a method can be marked as
            // GuaranteeNonBlocking as an assertion that the method doesn't block despite calling a blocking method.
            //
            // Methods that don't call blocking methods and aren't marked as Blocking are assumed not to block, so
            // they do not require this attribute.
            [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class GuaranteeNonBlockingAttribute : Attribute
            {
                public GuaranteeNonBlockingAttribute()
                {
                }
            }

            [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class NonThrowingAttribute : Attribute
            {
                public NonThrowingAttribute()
                {
                }
            }

            [SuppressMessage(FxCop.Category.Performance, "CA1813:AvoidUnsealedAttributes",
                Justification = "This is intended to be an attribute heirarchy. It does not affect product perf.")]
            [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor,
                AllowMultiple = true, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public class ThrowsAttribute : Attribute
            {
                readonly Type exceptionType;
                readonly string diagnosis;

                public ThrowsAttribute(Type exceptionType, string diagnosis)
                {
                    if (exceptionType == null)
                    {
                        throw Fx.Exception.ArgumentNull("exceptionType");
                    }
                    if (string.IsNullOrEmpty(diagnosis))
                    {
                        throw Fx.Exception.ArgumentNullOrEmpty("diagnosis");
                    }

                    this.exceptionType = exceptionType;
                    this.diagnosis = diagnosis;
                }

                public Type ExceptionType
                {
                    get
                    {
                        return this.exceptionType;
                    }
                }

                public string Diagnosis
                {
                    get
                    {
                        return this.diagnosis;
                    }
                }
            }

            [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class InheritThrowsAttribute : Attribute
            {
                public InheritThrowsAttribute()
                {
                }

                public Type FromDeclaringType
                {
                    get;
                    set;
                }
                public string From
                {
                    get;
                    set;
                }
            }

            [AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Module | AttributeTargets.Class |
                AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Constructor | AttributeTargets.Method |
                AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Event | AttributeTargets.Interface |
                AttributeTargets.Delegate, AllowMultiple = false, Inherited = false)]
            [Conditional("CODE_ANALYSIS")]
            public sealed class SecurityNoteAttribute : Attribute
            {
                public SecurityNoteAttribute()
                {
                }

                public string Critical
                {
                    get;
                    set;
                }
                public string Safe
                {
                    get;
                    set;
                }
                public string Miscellaneous
                {
                    get;
                    set;
                }
            }
        }
    }
}
