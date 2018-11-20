// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// make it convenient to deal with nullable types
    /// also set the default value if the field is null
    /// </summary>
    public static class Extensions
    {
#if DEBUG
        public static bool AmqpDebug = string.Equals(Environment.GetEnvironmentVariable("AMQP_DEBUG"), "1", StringComparison.Ordinal);
        public static Action<bool, AmqpConnection, Frame> FrameCallback;
        public static Action<string> TraceCallback;
#endif

        public static string GetString(this ArraySegment<byte> binary, int count = int.MaxValue, StringBuilder output = null)
        {
            if (binary.Count == 0)
            {
                return string.Empty;
            }

            StringBuilder sb = output ?? new StringBuilder(binary.Count * 2);
            for (int i = 0; i < Math.Min(count, binary.Count); ++i)
            {
                sb.AppendFormat(CultureInfo.InvariantCulture, "{0:X2}", binary.Array[binary.Offset + i]);
            }

            if (count < binary.Count)
            {
                sb.Append("...");
            }

            return output == null ? sb.ToString() : null;
        }

#if DEBUG
        public static void Trace(this ProtocolHeader header, bool send, AmqpConnection connection)
        {
            string message = string.Format(
                        "{0} [{1:X3}.{2:X3} {3:HH:mm:ss.fff}] {4} {5}",
                        AppDomain.CurrentDomain.FriendlyName,
                        Process.GetCurrentProcess().Id,
                        Environment.CurrentManagedThreadId,
                        DateTime.UtcNow,
                        send ? "SEND" : "RECV",
                        header.ToString());

            if (TraceCallback != null)
            {
                TraceCallback(message);
            }
            else
            {
                System.Diagnostics.Trace.WriteLine(message);
            }
        }

        public static void Trace(this Frame frame, bool send, AmqpConnection connection)
        {
            if (AmqpDebug)
            {
                if (FrameCallback != null)
                {
                    FrameCallback(send, connection, frame);
                }
                else
                {
                    StringBuilder sb = new StringBuilder(1024);
                    sb.AppendFormat(
                        "{0} [{1:X3}.{2:X3} {3:HH:mm:ss.fff}] {4} FRM({5:X4}|{6}|{7}|{8:X2}",
                        AppDomain.CurrentDomain.FriendlyName,
                        Process.GetCurrentProcess().Id,
                        Environment.CurrentManagedThreadId,
                        DateTime.UtcNow,
                        send ? "SEND" : "RECV",
                        frame.Size,
                        frame.DataOffset,
                        frame.Type,
                        frame.Channel);
                    if (frame.Command != null)
                    {
                        sb.Append(' ');
                        sb.Append(frame.Command);
                    }

                    if (frame.Payload.Count > 0)
                    {
                        sb.Append(' ');
                        frame.Payload.GetString(128, sb);
                    }

                    sb.Append(')');

                    if (TraceCallback != null)
                    {
                        TraceCallback(sb.ToString());
                    }
                    else
                    {
                        System.Diagnostics.Trace.WriteLine(sb.ToString());
                    }
                }
            }
        }
#endif // DEBUG

        // open
        public static uint MaxFrameSize(this Open open)
        {
            return open.MaxFrameSize == null ? uint.MaxValue : open.MaxFrameSize.Value;
        }

        public static ushort ChannelMax(this Open open)
        {
            return open.ChannelMax == null ? ushort.MaxValue : open.ChannelMax.Value;
        }

        public static uint IdleTimeOut(this Open open)
        {
            return open.IdleTimeOut == null || open.IdleTimeOut.Value == 0 ? uint.MaxValue : open.IdleTimeOut.Value;
        }

        // begin
        public static uint HandleMax(this Begin begin)
        {
            return begin.HandleMax == null ? uint.MaxValue : begin.HandleMax.Value;
        }

        public static uint OutgoingWindow(this Begin begin)
        {
            return begin.OutgoingWindow == null ? uint.MaxValue : begin.OutgoingWindow.Value;
        }

        public static uint IncomingWindow(this Begin begin)
        {
            return begin.IncomingWindow == null ? uint.MaxValue : begin.IncomingWindow.Value;
        }

        // attach
        public static bool IsReceiver(this Attach attach)
        {
            return attach.Role.HasValue && attach.Role.Value;
        }

        public static ulong MaxMessageSize(this Attach attach)
        {
            return attach.MaxMessageSize == null || attach.MaxMessageSize.Value == 0 ? ulong.MaxValue : attach.MaxMessageSize.Value;
        }

        public static Address Address(this Attach attach)
        {
            return Address(attach, attach.IsReceiver());
        }

        public static Address Address(this Attach attach, bool role)
        {
            if (role)
            {
                Fx.Assert(attach.Source != null && attach.Source is Source, "Source is not valid.");
                return ((Source)attach.Source).Address;
            }
            else
            {
                Fx.Assert(attach.Target != null && attach.Target is Target, "Target is not valid.");
                return ((Target)attach.Target).Address;
            }
        }

        public static SettleMode SettleType(this Attach attach)
        {
            SenderSettleMode ssm = attach.SndSettleMode.HasValue ? (SenderSettleMode)attach.SndSettleMode.Value : SenderSettleMode.Mixed;
            ReceiverSettleMode rsm = attach.RcvSettleMode.HasValue ? (ReceiverSettleMode)attach.RcvSettleMode.Value : ReceiverSettleMode.First;

            if (ssm == SenderSettleMode.Settled)
            {
                return SettleMode.SettleOnSend;
            }
            else
            {
                if (rsm == ReceiverSettleMode.First)
                {
                    return SettleMode.SettleOnReceive;
                }
                else
                {
                    return SettleMode.SettleOnDispose;
                }
            }
        }

        // transfer
        public static bool Settled(this Transfer transfer)
        {
            return transfer.Settled == null ? false : transfer.Settled.Value;
        }

        public static bool More(this Transfer transfer)
        {
            return transfer.More == null ? false : transfer.More.Value;
        }

        public static bool Resume(this Transfer transfer)
        {
            return transfer.Resume == null ? false : transfer.Resume.Value;
        }

        public static bool Aborted(this Transfer transfer)
        {
            return transfer.Aborted == null ? false : transfer.Aborted.Value;
        }

        public static bool Batchable(this Transfer transfer)
        {
            return transfer.Batchable == null ? false : transfer.Batchable.Value;
        }

        // disposition
        public static bool Settled(this Disposition disposition)
        {
            return disposition.Settled == null ? false : disposition.Settled.Value;
        }

        public static bool Batchable(this Disposition disposition)
        {
            return disposition.Batchable == null ? false : disposition.Batchable.Value;
        }

        // flow
        public static uint LinkCredit(this Flow flow)
        {
            return flow.LinkCredit.HasValue ? flow.LinkCredit.Value : uint.MaxValue;
        }

        public static bool Echo(this Flow flow)
        {
            return flow.Echo == null ? false : flow.Echo.Value;
        }

        // detach
        public static bool Closed(this Detach detach)
        {
            return detach.Closed == null ? false : detach.Closed.Value;
        }

        // message header
        public static bool Durable(this Header header)
        {
            return header.Durable.HasValue && header.Durable.Value;
        }

        public static byte Priority(this Header header)
        {
            return header.Priority == null ? (byte)0 : header.Priority.Value;
        }

        public static uint Ttl(this Header header)
        {
            return header.Ttl == null ? (uint)0 : header.Ttl.Value;
        }

        public static bool FirstAcquirer(this Header header)
        {
            return header.FirstAcquirer == null ? false : header.FirstAcquirer.Value;
        }

        public static uint DeliveryCount(this Header header)
        {
            return header.DeliveryCount == null ? (uint)0 : header.DeliveryCount.Value;
        }

        // message property
        public static DateTime AbsoluteExpiryTime(this Properties properties)
        {
            return properties.AbsoluteExpiryTime == null ? default(DateTime) : properties.AbsoluteExpiryTime.Value;
        }

        public static DateTime CreationTime(this Properties properties)
        {
            return properties.CreationTime == null ? default(DateTime) : properties.CreationTime.Value;
        }

        public static SequenceNumber GroupSequence(this Properties properties)
        {
            return properties.GroupSequence == null ? 0 : properties.GroupSequence.Value;
        }

        // delivery
        public static bool Transactional(this Delivery delivery)
        {
            return delivery.State != null && delivery.State.DescriptorCode == TransactionalState.Code;
        }

        public static bool IsReceivedDeliveryState(this Delivery delivery)
        {
            return delivery.State != null && delivery.State.DescriptorCode == Received.Code;
        }

        // Source and Target
        public static bool Dynamic(this Source source)
        {
            return source.Dynamic == null ? false : source.Dynamic.Value;
        }

        public static bool Dynamic(this Target target)
        {
            return target.Dynamic == null ? false : target.Dynamic.Value;
        }

        public static bool Durable(this Source source)
        {
            return source.Durable == null ? false : (TerminusDurability)source.Durable.Value == TerminusDurability.None;
        }

        public static bool Durable(this Target target)
        {
            return target.Durable == null ? false : (TerminusDurability)target.Durable.Value == TerminusDurability.None;
        }

        // settings
        public static void UpsertProperty(this Begin begin, AmqpSymbol symbol, object value)
        {
            if (begin.Properties == null)
            {
                begin.Properties = new Fields();
            }

            begin.Properties[symbol] = value;
        }

        public static void AddProperty(this Attach attach, AmqpSymbol symbol, object value)
        {
            if (attach.Properties == null)
            {
                attach.Properties = new Fields();
            }

            attach.Properties.Add(symbol, value);
        }

        public static void UpsertProperty(this Attach attach, AmqpSymbol symbol, object value)
        {
            if (attach.Properties == null)
            {
                attach.Properties = new Fields();
            }

            attach.Properties[symbol] = value;
        }

        public static void UpsertPropertyIfNotDefault<T>(this Attach attach, AmqpSymbol symbol, T value)
        {
            if (!object.Equals(value, default(T)))
            {
                if (attach.Properties == null)
                {
                    attach.Properties = new Fields();
                }

                attach.Properties[symbol] = value;
            }
        }

        // open 
        public static void AddProperty(this Open open, AmqpSymbol symbol, object value)
        {
            if (open.Properties == null)
            {
                open.Properties = new Fields();
            }

            open.Properties.Add(symbol, value);
        }

        public static TValue GetSettingPropertyOrDefault<TValue>(this AmqpLink thisPtr, AmqpSymbol key, TValue defaultValue)
        {
            TValue value;
            if (thisPtr != null && thisPtr.Settings != null && thisPtr.Settings.Properties != null && thisPtr.Settings.Properties.TryGetValue<TValue>(key, out value))
            {
                return value;
            }
            else
            {
                return defaultValue;
            }
        }

        public static TValue ExtractSettingPropertyValueOrDefault<TValue>(this AmqpLink thisPtr, AmqpSymbol key, TValue defaultValue)
        {
            TValue value;
            if (thisPtr != null && thisPtr.Settings != null && thisPtr.Settings.Properties != null && thisPtr.Settings.Properties.TryRemoveValue<TValue>(key, out value))
            {
                return value;
            }
            else
            {
                return defaultValue;
            }
        }

        public static void AddExtension(this AmqpConnection connection, object extension)
        {
            connection.Extensions.Add(extension.GetType(), extension);
        }

        public static bool TryGetExtension<T>(this AmqpConnection connection, out T extension)
        {
            if (connection.Extensions.TryGetValue(typeof(T), out object obj))
            {
                extension = (T)obj;
                return true;
            }

            extension = default(T);
            return false;
        }

        public static ArraySegment<byte> AsSegment(this ByteBuffer buffer)
        {
            if (buffer == null)
            {
                return default(ArraySegment<byte>);
            }

            return new ArraySegment<byte>(buffer.Buffer, buffer.Offset, buffer.Length);
        }
    }
}