﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System.Collections.Concurrent;
    using System.Diagnostics;
    using System.Globalization;
    using System.Threading;
    using Microsoft.Azure.Amqp.Framing;

    static class AmqpDebug
    {
        [Conditional("AMQP_DEBUG")]
        public static void Log(object source, bool send, Performative command)
        {
#if AMQP_DEBUG
            AmqpDebugImpl.Log(source, send, command);
#endif
        }

        [Conditional("AMQP_DEBUG")]
        public static void Log(object source, bool send, ulong code, uint p1, uint p2)
        {
#if AMQP_DEBUG
            AmqpDebugImpl.Log(source, send, code, p1, p2);
#endif
        }

        [Conditional("AMQP_DEBUG")]
        public static void Dump(object source)
        {
#if AMQP_DEBUG
            AmqpDebugImpl.Dump(source);
#endif
        }

#if AMQP_DEBUG
        struct Entry
        {
            public long Ticks;
            public int ThreadId;
            public bool Send;
            public ulong Code;
            public uint Param1;
            public uint Param2;
        }

        sealed class AmqpDebugImpl
        {
            const int DefaultSize = 500 * 1024;
            static readonly ConcurrentDictionary<string, AmqpDebugImpl> instances = new ConcurrentDictionary<string, AmqpDebugImpl>();

            readonly Entry[] entries;
            int index;

            AmqpDebugImpl(int size)
            {
                this.index = -1;
                this.entries = new Entry[size];
            }

            public static void Log(object source, bool send, Performative command)
            {
                AmqpDebugImpl instance = AmqpDebugImpl.GetInstance(source);
                ulong code = command.DescriptorCode;
                uint p1 = 0;
                uint p2 = 0;
                if (code == Transfer.Code)
                {
                    Transfer transfer = (Transfer)command;
                    p1 = transfer.DeliveryId ?? 0;
                    p2 = transfer.Settled() ? 1u : 0u;
                }
                else if (code == Disposition.Code)
                {
                    Disposition disp = (Disposition)command;
                    p1 = disp.First ?? 0;
                    p2 = disp.Last ?? p1;
                }
                else if (code == Flow.Code)
                {
                    Flow flow = (Flow)command;
                    p1 = flow.IncomingWindow ?? 0;
                    p2 = flow.NextIncomingId ?? 0;
                    if (flow.Handle.HasValue)
                    {
                        instance.LogInternal(send, code, flow.DeliveryCount.Value, flow.LinkCredit.Value);
                    }
                }

                instance.LogInternal(send, code, p1, p2);
            }

            public static void Log(object source, bool send, ulong code, uint p1, uint p2)
            {
                AmqpDebugImpl instance = AmqpDebugImpl.GetInstance(source);
                instance.LogInternal(send, code, p1, p2);
            }

            public static void Dump(object source)
            {
                AmqpDebugImpl instance;
                string key = source.ToString();
                if (instances != null && instances.TryRemove(key, out instance))
                {
                    try
                    {
                        instance.DumpInternal(key + ".log");
                    }
                    catch
                    {
                    }
                }
            }

            static AmqpDebugImpl GetInstance(object source)
            {
                string key = source.ToString();
                AmqpDebugImpl instance;
                if (!instances.TryGetValue(key, out instance))
                {
                    instance = instances.GetOrAdd(key, new AmqpDebugImpl(DefaultSize));
                }

                return instance;
            }

            void LogInternal(bool send, ulong code, uint p1, uint p2)
            {
                int p = (int)((uint)Interlocked.Increment(ref this.index) % this.entries.Length);
                this.entries[p] = new Entry()
                {
                    Ticks = Stopwatch.GetTimestamp(),
                    ThreadId = Thread.CurrentThread.ManagedThreadId,
                    Send = send,
                    Code = code,
                    Param1 = p1,
                    Param2 = p2
                };
            }

            void DumpInternal(string file)
            {
                using (var sw = new System.IO.StreamWriter(file, false))
                {
                    sw.WriteLine(string.Format(CultureInfo.InvariantCulture, "ticks({0})\tdirection\top\tp1\tp2", System.TimeSpan.FromMilliseconds(1).Ticks));
                    int p = this.index;
                    for (int i = 0; i < this.entries.Length; ++i)
                    {
                        var t = this.entries[++p % this.entries.Length];
                        if (t.Ticks > 0)
                        {
                            sw.WriteLine(
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    "{0}\t{1}\t{2}\t{3}\t{4}\t{5}",
                                    t.Ticks,
                                    t.ThreadId,
                                    t.Send ? "SEND" : "RECV",
                                    t.Code,
                                    t.Param1,
                                    t.Param2));
                        }
                    }
                }
            }
        }
#endif
    }
}
