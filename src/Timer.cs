// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;

    public interface ITimer
    {
        ITimer Set(TimeSpan delay);

        bool Cancel();
    }

    public interface ITimerFactory
    {
        ITimer Create(TimerCallback timerCallback, object state, TimeSpan delay);
    }

    sealed class SystemTimerFactory : ITimerFactory
    {
        public static ITimerFactory Default = new SystemTimerFactory();

        public ITimer Create(TimerCallback timerCallback, object state, TimeSpan delay)
        {
            return new SystemTimer(timerCallback, state, delay);
        }
    }

    sealed class SystemTimer : ITimer
    {
        readonly Timer timer;

        public SystemTimer(TimerCallback timerCallback, object state, TimeSpan delay)
        {
            this.timer = new Timer(timerCallback, state, delay, Timeout.InfiniteTimeSpan);
        }

        public ITimer Set(TimeSpan delay)
        {
            this.timer.Change(delay, Timeout.InfiniteTimeSpan);
            return this;
        }

        public bool Cancel()
        {
            return this.timer.Change(Timeout.Infinite, Timeout.Infinite);
        }
    }
}
