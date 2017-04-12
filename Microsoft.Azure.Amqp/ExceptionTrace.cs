// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.Tracing;
    using System.Globalization;
    using System.Runtime.Versioning;
    using System.Text;
    using Microsoft.Azure.Amqp.Tracing;

    class ExceptionTrace
    {
        const ushort FailFastEventLogCategory = 6;
        readonly string eventSourceName;

        public ExceptionTrace(string eventSourceName)
        {
            this.eventSourceName = eventSourceName;
        }

        public Exception AsError(Exception exception, EventTraceActivity activity = null)
        {
            return TraceException<Exception>(exception, EventLevel.Error, activity);
        }

        public Exception AsInformation(Exception exception, EventTraceActivity activity = null)
        {
            return TraceException<Exception>(exception, EventLevel.Informational, activity);
        }

        public Exception AsWarning(Exception exception, EventTraceActivity activity = null)
        {
            return TraceException<Exception>(exception, EventLevel.Warning, activity);
        }

        public Exception AsVerbose(Exception exception, EventTraceActivity activity = null)
        {
            return TraceException<Exception>(exception, EventLevel.Verbose, activity);
        }

        public ArgumentException Argument(string paramName, string message)
        {
            return TraceException<ArgumentException>(new ArgumentException(message, paramName), EventLevel.Error);
        }

        public ArgumentNullException ArgumentNull(string paramName)
        {
            return TraceException<ArgumentNullException>(new ArgumentNullException(paramName), EventLevel.Error);
        }

        public ArgumentNullException ArgumentNull(string paramName, string message)
        {
            return TraceException<ArgumentNullException>(new ArgumentNullException(paramName, message), EventLevel.Error);
        }

        public ArgumentException ArgumentNullOrEmpty(string paramName)
        {
            return this.Argument(paramName, CommonResources.GetString(CommonResources.ArgumentNullOrEmpty, paramName));
        }

        public ArgumentException ArgumentNullOrWhiteSpace(string paramName)
        {
            return this.Argument(paramName, CommonResources.GetString(CommonResources.ArgumentNullOrWhiteSpace, paramName));
        }

        public ArgumentOutOfRangeException ArgumentOutOfRange(string paramName, object actualValue, string message)
        {
            return TraceException<ArgumentOutOfRangeException>(new ArgumentOutOfRangeException(paramName, actualValue, message), EventLevel.Error);
        }

        // When throwing ObjectDisposedException, it is highly recommended that you use this ctor
        // [C#]
        // public ObjectDisposedException(string objectName, string message);
        // And provide null for objectName but meaningful and relevant message for message. 
        // It is recommended because end user really does not care or can do anything on the disposed object, commonly an internal or private object.
        public ObjectDisposedException ObjectDisposed(string message)
        {
            // pass in null, not disposedObject.GetType().FullName as per the above guideline
            return TraceException<ObjectDisposedException>(new ObjectDisposedException(null, message), EventLevel.Error);
        }

        public void TraceHandled(Exception exception, string catchLocation, EventTraceActivity activity = null)
        {
#if DEBUG
            Debug.WriteLine(string.Format(
                CultureInfo.InvariantCulture,
                "IotHub/TraceHandled ThreadID=\"{0}\" catchLocation=\"{1}\" exceptionType=\"{2}\" exception=\"{3}\"",
                Environment.CurrentManagedThreadId,
                catchLocation,
                exception.GetType(),
                exception.ToStringSlim()));
#endif
        }

        public void TraceUnhandled(Exception exception)
        {
        }

#if !NETSTANDARD && !PCL
        [ResourceConsumption(ResourceScope.Process)]
#endif
        [Fx.Tag.SecurityNote(Critical = "Calls 'System.Runtime.Interop.UnsafeNativeMethods.IsDebuggerPresent()' which is a P/Invoke method",
        Safe = "Does not leak any resource, needed for debugging")]
        public TException TraceException<TException>(TException exception, EventLevel level, EventTraceActivity activity = null)
            where TException : Exception
        {
            if (!exception.Data.Contains(this.eventSourceName))
            {
                // Only trace if this is the first time an exception is thrown by this ExceptionTrace/EventSource.
                exception.Data[this.eventSourceName] = this.eventSourceName;

                switch (level)
                {
                    case EventLevel.Critical:
                    case EventLevel.Error:
#if NETSTANDARD || PCL
                        Debug.WriteLine("[{0}] An Exception is being thrown: {1}", level, exception);
#else
                        Trace.TraceError("An Exception is being thrown: {0}", GetDetailsForThrownException(exception));
#endif
                        ////if (MessagingClientEtwProvider.Provider.IsEnabled(
                        ////        EventLevel.Error,
                        ////        MessagingClientEventSource.Keywords.Client,
                        ////        MessagingClientEventSource.Channels.DebugChannel))
                        ////{
                        ////    MessagingClientEtwProvider.Provider.ThrowingExceptionError(activity, GetDetailsForThrownException(exception));
                        ////}

                        break;
                    case EventLevel.Warning:
#if NETSTANDARD || PCL
                        Debug.WriteLine("[{0}] An Exception is being thrown: {1}", level, exception);
#else
                        Trace.TraceWarning("An Exception is being thrown: {0}", GetDetailsForThrownException(exception));
#endif
                        ////if (MessagingClientEtwProvider.Provider.IsEnabled(
                        ////        EventLevel.Warning,
                        ////        MessagingClientEventSource.Keywords.Client,
                        ////        MessagingClientEventSource.Channels.DebugChannel))
                        ////{
                        ////    MessagingClientEtwProvider.Provider.ThrowingExceptionWarning(activity, GetDetailsForThrownException(exception));
                        ////}

                        break;
                    default:
#if DEBUG
                        ////if (MessagingClientEtwProvider.Provider.IsEnabled(
                        ////        EventLevel.Verbose,
                        ////        MessagingClientEventSource.Keywords.Client,
                        ////        MessagingClientEventSource.Channels.DebugChannel))
                        ////{
                        ////    MessagingClientEtwProvider.Provider.ThrowingExceptionVerbose(activity, GetDetailsForThrownException(exception));
                        ////}
#endif

                        break;
                }
            }

            return exception;
        }

        public static string GetDetailsForThrownException(Exception e)
        {
#if !PCL
            StringBuilder details = new StringBuilder(2048);
            details.AppendLine(e.ToStringSlim());
            if (string.IsNullOrWhiteSpace(e.StackTrace))
            {
                // Include the current callstack (this ensures we see the Stack in case exception is not output when caught)
                const int MaxStackTraceLength = 2000;
                string stackTraceString = Environment.StackTrace;
                if (stackTraceString.Length > MaxStackTraceLength)
                {
                    stackTraceString = stackTraceString.Substring(0, MaxStackTraceLength) + "...";
                }

                details.Append(stackTraceString);
            }

            return details.ToString();
#else
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
#endif
        }
    }
}
