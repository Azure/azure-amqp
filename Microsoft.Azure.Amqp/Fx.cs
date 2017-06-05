// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Threading;

    static class Fx
    {
        // This is only used for EventLog Source therefore matching EventLog source rather than ETL source
        const string DefaultEventSource = "Microsoft.DeviceHub";

#if DEBUG
        const string SBRegistryKey = @"SOFTWARE\Microsoft\DeviceHub\v2.0";
        const string AssertsFailFastName = "AssertsFailFast";
        const string BreakOnExceptionTypesName = "BreakOnExceptionTypes";
        const string FastDebugName = "FastDebug";
#endif

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
            throw new AssertionFailedException(description);
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
            throw new FatalException(description);
        }

        public static bool IsFatal(Exception exception)
        {
#if PCL
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
#else
            while (exception != null)
            {
                // FYI, CallbackException is-a FatalException
                if (exception is FatalException ||
#if NET45 || MONOANDROID
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
                    break;
                }
                else
                {
                    break;
                }
            }

            return false;
#endif
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
