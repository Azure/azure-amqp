// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This limited implementation of the System.Threading.Timer is necessary for PCL profile111.

namespace System.Threading
{
    internal delegate void TimerCallback(object state);

    internal sealed class Timer : CancellationTokenSource, IDisposable
    {
        internal Timer(TimerCallback callback, object state, TimeSpan dueTime, TimeSpan period) :
            this(callback, state, (int)dueTime.TotalMilliseconds, (int)period.TotalMilliseconds)
        {
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }

        internal Timer(TimerCallback callback, object state, int dueTime, int period)
        {
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }

        internal bool Change(TimeSpan dueTime, TimeSpan period)
        {
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }

        internal bool Change(int dueTime, int period)
        {
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }

        public new void Dispose() { base.Cancel(); }
    }
}
