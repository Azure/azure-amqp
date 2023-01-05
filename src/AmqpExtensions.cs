// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transaction;

    /// <summary>
    /// Defines the extension methods used by the library.
    /// </summary>
    public static class Extensions
    {
        // methods for tracing
        internal static string GetString(this ArraySegment<byte> binary, int count = int.MaxValue, StringBuilder output = null)
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

        internal static string GetString(IDictionary<ArraySegment<byte>, Delivery> deliveries)
        {
            StringBuilder sb = new StringBuilder();
            foreach (var item in deliveries)
            {
                sb.Append(Extensions.GetString(item.Key));
                sb.Append(":");
                sb.Append(item.Value.State);
                sb.Append(",");
            }

            return sb.ToString();
        }

        // open
        /// <summary>
        /// Gets the value of open.max-frame-size or uint.MaxValue if it is not set.
        /// </summary>
        /// <param name="open">The <see cref="Open"/> performative.</param>
        /// <returns>The value of max-frame-size.</returns>
        public static uint MaxFrameSize(this Open open)
        {
            return open.MaxFrameSize == null ? uint.MaxValue : open.MaxFrameSize.Value;
        }

        /// <summary>
        /// Gets the value of open.channel-max or ushort.MaxValue it is not set.
        /// </summary>
        /// <param name="open">The <see cref="Open"/> performative.</param>
        /// <returns>The value of channel-max.</returns>
        public static ushort ChannelMax(this Open open)
        {
            return open.ChannelMax == null ? ushort.MaxValue : open.ChannelMax.Value;
        }

        /// <summary>
        /// Gets the value of open.idle-time-out or uint.MaxValue it is not set or its value is 0.
        /// </summary>
        /// <param name="open">The <see cref="Open"/> performative.</param>
        /// <returns>The value of idle-time-out.</returns>
        public static uint IdleTimeOut(this Open open)
        {
            return open.IdleTimeOut == null || open.IdleTimeOut.Value == 0 ? uint.MaxValue : open.IdleTimeOut.Value;
        }

        // begin
        /// <summary>
        /// Gets the value of begin.handle-max or uint.MaxValue it is not set.
        /// </summary>
        /// <param name="begin">The <see cref="Begin"/> performative.</param>
        /// <returns>The value of handle-max.</returns>
        public static uint HandleMax(this Begin begin)
        {
            return begin.HandleMax == null ? uint.MaxValue : begin.HandleMax.Value;
        }

        /// <summary>
        /// Gets the value of begin.outgoing-window or uint.MaxValue it is not set.
        /// </summary>
        /// <param name="begin">The <see cref="Begin"/> performative.</param>
        /// <returns>The value of outgoing-window.</returns>
        public static uint OutgoingWindow(this Begin begin)
        {
            return begin.OutgoingWindow == null ? uint.MaxValue : begin.OutgoingWindow.Value;
        }

        /// <summary>
        /// Gets the value of begin.incoming-window or uint.MaxValue it is not set.
        /// </summary>
        /// <param name="begin">The <see cref="Begin"/> performative.</param>
        /// <returns>The value of incoming-window.</returns>
        public static uint IncomingWindow(this Begin begin)
        {
            return begin.IncomingWindow == null ? uint.MaxValue : begin.IncomingWindow.Value;
        }

        // attach
        /// <summary>
        /// Gets the value of attach.role or false it is not set.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <returns>The value of role.</returns>
        public static bool IsReceiver(this Attach attach)
        {
            return attach.Role.HasValue && attach.Role.Value;
        }

        /// <summary>
        /// Gets the value of attach.max-message-size or false it is not set.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <returns>The value of max-message-size.</returns>
        public static ulong MaxMessageSize(this Attach attach)
        {
            return attach.MaxMessageSize == null || attach.MaxMessageSize.Value == 0 ? ulong.MaxValue : attach.MaxMessageSize.Value;
        }

        /// <summary>
        /// Gets the address from attach.source if attach.role is true, or attach.target otherwise.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <returns>The address.</returns>
        public static Address Address(this Attach attach)
        {
            return Address(attach, attach.IsReceiver());
        }

        /// <summary>
        /// Gets the address from attach for the given role.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <param name="role">true for source and false for target</param>
        /// <returns></returns>
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

        /// <summary>
        /// Gets a SettleMode based on the <see cref="Attach.SndSettleMode"/> and <see cref="Attach.RcvSettleMode"/>.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <returns>A <see cref="SettleMode"/>.</returns>
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
        /// <summary>
        /// Gets the value of transfer.settled or false if it is not set.
        /// </summary>
        /// <param name="transfer">The <see cref="Transfer"/> performative.</param>
        /// <returns>true if transfer is settled or false otherwise.</returns>
        public static bool Settled(this Transfer transfer)
        {
            return transfer.Settled == null ? false : transfer.Settled.Value;
        }

        /// <summary>
        /// Gets the value of transfer.more or false if it is not set.
        /// </summary>
        /// <param name="transfer">The <see cref="Transfer"/> performative.</param>
        /// <returns>true if transfer not the last or false otherwise.</returns>
        public static bool More(this Transfer transfer)
        {
            return transfer.More == null ? false : transfer.More.Value;
        }

        /// <summary>
        /// Gets the value of transfer.resume or false if it is not set.
        /// </summary>
        /// <param name="transfer">The <see cref="Transfer"/> performative.</param>
        /// <returns>true if transfer is resuming or false otherwise.</returns>
        public static bool Resume(this Transfer transfer)
        {
            return transfer.Resume == null ? false : transfer.Resume.Value;
        }

        /// <summary>
        /// Gets the value of transfer.aborted or false if it is not set.
        /// </summary>
        /// <param name="transfer">The <see cref="Transfer"/> performative.</param>
        /// <returns>true if transfer is aborted or false otherwise.</returns>
        public static bool Aborted(this Transfer transfer)
        {
            return transfer.Aborted == null ? false : transfer.Aborted.Value;
        }

        /// <summary>
        /// Gets the value of transfer.batchable or false if it is not set.
        /// </summary>
        /// <param name="transfer">The <see cref="Transfer"/> performative.</param>
        /// <returns>true if transfer is batchable or false otherwise.</returns>
        public static bool Batchable(this Transfer transfer)
        {
            return transfer.Batchable == null ? false : transfer.Batchable.Value;
        }

        // disposition
        /// <summary>
        /// Gets the value of disposition.settled or false if it is not set.
        /// </summary>
        /// <param name="disposition">The <see cref="Disposition"/> performative.</param>
        /// <returns>true if disposition is settled or false otherwise.</returns>
        public static bool Settled(this Disposition disposition)
        {
            return disposition.Settled == null ? false : disposition.Settled.Value;
        }

        /// <summary>
        /// Gets the value of disposition.batchable or false if it is not set.
        /// </summary>
        /// <param name="disposition">The <see cref="Disposition"/> performative.</param>
        /// <returns>true if disposition is batchable or false otherwise.</returns>
        public static bool Batchable(this Disposition disposition)
        {
            return disposition.Batchable == null ? false : disposition.Batchable.Value;
        }

        // flow
        /// <summary>
        /// Gets the value of flow.link-credit or uint.MaxValue if it is not set.
        /// </summary>
        /// <param name="flow">The <see cref="Flow"/> performative.</param>
        /// <returns>The link-credit of the flow.</returns>
        public static uint LinkCredit(this Flow flow)
        {
            return flow.LinkCredit.HasValue ? flow.LinkCredit.Value : uint.MaxValue;
        }

        /// <summary>
        /// Gets the value of flow.echo or false if it is not set.
        /// </summary>
        /// <param name="flow">The <see cref="Flow"/> performative.</param>
        /// <returns>The echo of the flow.</returns>
        public static bool Echo(this Flow flow)
        {
            return flow.Echo == null ? false : flow.Echo.Value;
        }

        // detach
        /// <summary>
        /// Gets the value of detach.closed or false if it is not set.
        /// </summary>
        /// <param name="detach">The <see cref="Detach"/> performative.</param>
        /// <returns>true if link is closed or false otherwise.</returns>
        public static bool Closed(this Detach detach)
        {
            return detach.Closed == null ? false : detach.Closed.Value;
        }

        // message header
        /// <summary>
        /// Gets the value of header.durable or false if it is not set.
        /// </summary>
        /// <param name="header">The <see cref="Header"/> section.</param>
        /// <returns>true if message is durable or false otherwise.</returns>
        public static bool Durable(this Header header)
        {
            return header.Durable.HasValue && header.Durable.Value;
        }

        /// <summary>
        /// Gets the value of header.priority or 0 if it is not set.
        /// </summary>
        /// <param name="header">The <see cref="Header"/> section.</param>
        /// <returns>priority of the message.</returns>
        public static byte Priority(this Header header)
        {
            return header.Priority == null ? (byte)0 : header.Priority.Value;
        }

        /// <summary>
        /// Gets the value of header.ttl or 0 if it is not set.
        /// </summary>
        /// <param name="header">The <see cref="Header"/> section.</param>
        /// <returns>ttl of the message.</returns>
        public static uint Ttl(this Header header)
        {
            return header.Ttl == null ? (uint)0 : header.Ttl.Value;
        }

        /// <summary>
        /// Gets the value of header.first-acquirer or false if it is not set.
        /// </summary>
        /// <param name="header">The <see cref="Header"/> section.</param>
        /// <returns>first-acquirer of the message.</returns>
        public static bool FirstAcquirer(this Header header)
        {
            return header.FirstAcquirer == null ? false : header.FirstAcquirer.Value;
        }

        /// <summary>
        /// Gets the value of header.delivery-count or 0 if it is not set.
        /// </summary>
        /// <param name="header">The <see cref="Header"/> section.</param>
        /// <returns>delivery-count of the message.</returns>
        public static uint DeliveryCount(this Header header)
        {
            return header.DeliveryCount == null ? (uint)0 : header.DeliveryCount.Value;
        }

        // message property
        /// <summary>
        /// Gets the value of properties.absolute-expiry-time or DateTime.MinValue if it is not set.
        /// </summary>
        /// <param name="properties">The <see cref="Properties"/> section.</param>
        /// <returns>absolute-expiry-time of the message.</returns>
        public static DateTime AbsoluteExpiryTime(this Properties properties)
        {
            return properties.AbsoluteExpiryTime == null ? default(DateTime) : properties.AbsoluteExpiryTime.Value;
        }

        /// <summary>
        /// Gets the value of properties.creation-time or DateTime.MinValue if it is not set.
        /// </summary>
        /// <param name="properties">The <see cref="Properties"/> section.</param>
        /// <returns>creation-time of the message.</returns>
        public static DateTime CreationTime(this Properties properties)
        {
            return properties.CreationTime == null ? default(DateTime) : properties.CreationTime.Value;
        }

        /// <summary>
        /// Gets the value of properties.group-sequence or 0 if it is not set.
        /// </summary>
        /// <param name="properties">The <see cref="Properties"/> section.</param>
        /// <returns>group-sequence of the message.</returns>
        public static SequenceNumber GroupSequence(this Properties properties)
        {
            return properties.GroupSequence == null ? 0 : properties.GroupSequence.Value;
        }

        // delivery
        /// <summary>
        /// Gets the value that indicates if the delivery is part of a transaction.
        /// </summary>
        /// <param name="deliveryState">The <see cref="DeliveryState"/>.</param>
        /// <returns>true if the delivery is transactional or false otherwise.</returns>
        public static bool Transactional(this DeliveryState deliveryState)
        {
            return deliveryState != null && deliveryState.DescriptorCode == TransactionalState.Code;
        }

        /// <summary>
        /// Examines if the delivery has a <see cref="Received"/> state.
        /// </summary>
        /// <param name="delivery">The <see cref="Delivery"/>.</param>
        /// <returns>true if state is <see cref="Received"/> or false otherwise.</returns>
        public static bool IsReceivedDeliveryState(this Delivery delivery)
        {
            return delivery.State != null && delivery.State.DescriptorCode == Received.Code;
        }

        // Source and Target
        /// <summary>
        /// Gets the value of source.dynamic or false if it is not set.
        /// </summary>
        /// <param name="source">The <see cref="Source"/> object.</param>
        /// <returns>true if source is dynamic or false otherwise.</returns>
        public static bool Dynamic(this Source source)
        {
            return source.Dynamic == null ? false : source.Dynamic.Value;
        }

        /// <summary>
        /// Gets the value of target.dynamic or false if it is not set.
        /// </summary>
        /// <param name="target">The <see cref="Target"/> object.</param>
        /// <returns>true if target is dynamic or false otherwise.</returns>
        public static bool Dynamic(this Target target)
        {
            return target.Dynamic == null ? false : target.Dynamic.Value;
        }

        /// <summary>
        /// Gets the value of source.durable or false if it is not set.
        /// </summary>
        /// <param name="source">The <see cref="Source"/> object.</param>
        /// <returns>true if source is durable or false otherwise.</returns>
        public static bool Durable(this Source source)
        {
            return source.Durable == null ? false : (TerminusDurability)source.Durable.Value == TerminusDurability.None;
        }

        /// <summary>
        /// Gets the value of target.durable or false if it is not set.
        /// </summary>
        /// <param name="target">The <see cref="Target"/> object.</param>
        /// <returns>true if target is durable or false otherwise.</returns>
        public static bool Durable(this Target target)
        {
            return target.Durable == null ? false : (TerminusDurability)target.Durable.Value == TerminusDurability.None;
        }

        /// <summary>
        /// Returns true if the given delivery state has reached a terminal outcome.
        /// </summary>
        /// <param name="deliveryState">The <see cref="DeliveryState"/> to check if it has reached an outcome.</param>
        public static bool IsTerminal(this DeliveryState deliveryState)
        {
            return deliveryState != null &&
                (deliveryState.DescriptorCode == Accepted.Code ||
                deliveryState.DescriptorCode == Modified.Code ||
                deliveryState.DescriptorCode == Rejected.Code ||
                deliveryState.DescriptorCode == Released.Code);
        }

        /// <summary>
        /// Return the ExpiryPolicy from the link settings by checking the Target or Source, depending if the link is a receiver or sender.
        /// </summary>
        /// <param name="linkSettings">The link settings to obtain the ExpiryPolicy from.</param>
        public static AmqpSymbol GetExpiryPolicy(this AmqpLinkSettings linkSettings)
        {
            AmqpSymbol? expiryPolicy;
            if (linkSettings.IsReceiver())
            {
                var target = linkSettings.Target as Target;
                expiryPolicy = target?.ExpiryPolicy;
            }
            else
            {
                var source = linkSettings.Source as Source;
                expiryPolicy = source?.ExpiryPolicy;
            }

            return expiryPolicy ?? new AmqpSymbol();
        }

        /// <summary>
        /// Set the ExpiryPolicy field for the given link settings on the Target or Source, depending if the link is a receiver or sender.
        /// </summary>
        /// <param name="linkSettings">The link settings to set the expiry policy on.</param>
        /// <param name="expiryPolicy">The expiryPolicy to be set.</param>
        public static void SetExpiryPolicy(this AmqpLinkSettings linkSettings, LinkTerminusExpiryPolicy expiryPolicy)
        {
            if (linkSettings.IsReceiver() && linkSettings.Target is Target target)
            {
                target.ExpiryPolicy = expiryPolicy.GetExpiryPolicySymbol();
            }
            else if (!linkSettings.IsReceiver() && linkSettings.Source is Source source)
            {
                source.ExpiryPolicy = expiryPolicy.GetExpiryPolicySymbol();
            }
            else
            {
                throw new InvalidOperationException($"Cannot set expiration policy because the {(linkSettings.IsReceiver() ? nameof(Target) : nameof(Source))} is null.");
            }
        }

        /// <summary>
        /// Return the terminus expiry timeout from the link settings by checking the Target or Source, depending if the link is a receiver or sender.
        /// </summary>
        /// <param name="linkSettings">The link settings to obtain the terminus expiry timeout from.</param>
        public static TimeSpan GetExpiryTimeout(this AmqpLinkSettings linkSettings)
        {
            uint? timeoutInSeconds;
            if (linkSettings.IsReceiver())
            {
                var target = linkSettings.Target as Target;
                timeoutInSeconds = target?.Timeout;
            }
            else
            {
                var source = linkSettings.Source as Source;
                timeoutInSeconds = source?.Timeout;
            }

            if (timeoutInSeconds == null)
            {
                timeoutInSeconds = 0;
            }
            return TimeSpan.FromSeconds((uint)timeoutInSeconds);
        }

        /// <summary>
        /// Set the Timeout field for the given link settings on the Target or Source, depending if the link is a receiver or sender.
        /// </summary>
        /// <param name="linkSettings">The link settings to set the expiry timeout on.</param>
        /// <param name="expirationTimeout">The expiry timeout to be set.</param>
        public static void SetExpiryTimeout(this AmqpLinkSettings linkSettings, TimeSpan expirationTimeout)
        {
            if (linkSettings.IsReceiver() && linkSettings.Target is Target target)
            {
                target.Timeout = Convert.ToUInt32(expirationTimeout.TotalSeconds);
            }
            else if (!linkSettings.IsReceiver() && linkSettings.Source is Source source)
            {
                source.Timeout = Convert.ToUInt32(expirationTimeout.TotalSeconds);
            }
            else
            {
                throw new InvalidOperationException($"Cannot set expiration timeout because the {(linkSettings.IsReceiver() ? nameof(Target) : nameof(Source))} is null.");
            }
        }

        /// <summary>
        /// Returns the corresponding <see cref="AmqpSymbol"/> for the given link terminus expiry policy.
        /// </summary>
        public static AmqpSymbol GetExpiryPolicySymbol(this LinkTerminusExpiryPolicy linkTerminusExpiryPolicy)
        {
            switch (linkTerminusExpiryPolicy)
            {
                case LinkTerminusExpiryPolicy.Link_Detach:
                    return TerminusExpiryPolicy.LinkDetach;
                case LinkTerminusExpiryPolicy.Session_End:
                    return TerminusExpiryPolicy.SessionEnd;
                case LinkTerminusExpiryPolicy.Connection_Close:
                    return TerminusExpiryPolicy.ConnectionClose;
                case LinkTerminusExpiryPolicy.Never:
                    return TerminusExpiryPolicy.Never;
                default:
                    return new AmqpSymbol(null);
            }
        }

        /// <summary>
        /// Checks if the given link is closed due to link stealing.
        /// </summary>
        /// <param name="link">The link to be checked.</param>
        /// <returns>True if the link has been closed due to link stealing.</returns>
        public static bool IsStolen(this AmqpLink link)
        {
            return link.IsClosing() && link.TerminalException is AmqpException amqpException && AmqpErrorCode.Stolen.Equals(amqpException.Error.Condition);
        }

        /// <summary>
        /// Updates or inserts a value in begin.properties.
        /// </summary>
        /// <param name="begin">The <see cref="Begin"/> performative.</param>
        /// <param name="symbol">The symbol key.</param>
        /// <param name="value">The property value.</param>
        public static void UpsertProperty(this Begin begin, AmqpSymbol symbol, object value)
        {
            if (begin.Properties == null)
            {
                begin.Properties = new Fields();
            }

            begin.Properties[symbol] = value;
        }

        /// <summary>
        /// Adds a value in attach.properties.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <param name="symbol">The symbol key.</param>
        /// <param name="value">The property value.</param>
        public static void AddProperty(this Attach attach, AmqpSymbol symbol, object value)
        {
            if (attach.Properties == null)
            {
                attach.Properties = new Fields();
            }

            attach.Properties.Add(symbol, value);
        }

        /// <summary>
        /// Updates or inserts a value in attach.properties.
        /// </summary>
        /// <param name="attach">The <see cref="Attach"/> performative.</param>
        /// <param name="symbol">The symbol key.</param>
        /// <param name="value">The property value.</param>
        public static void UpsertProperty(this Attach attach, AmqpSymbol symbol, object value)
        {
            if (attach.Properties == null)
            {
                attach.Properties = new Fields();
            }

            attach.Properties[symbol] = value;
        }

        // open 
        /// <summary>
        /// Adds a value in open.properties.
        /// </summary>
        /// <param name="open">The <see cref="Open"/> performative.</param>
        /// <param name="symbol">The symbol key.</param>
        /// <param name="value">The property value.</param>
        public static void AddProperty(this Open open, AmqpSymbol symbol, object value)
        {
            if (open.Properties == null)
            {
                open.Properties = new Fields();
            }

            open.Properties.Add(symbol, value);
        }

        /// <summary>
        /// Gets a value from a link's properties or the default value if it does not exist.
        /// </summary>
        /// <typeparam name="TValue">The value type.</typeparam>
        /// <param name="thisPtr">The link object.</param>
        /// <param name="key">The key.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>The property value or the default.</returns>
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

        /// <summary>
        /// Gets and removes a value from a link's properties or the default value if it does not exist.
        /// </summary>
        /// <typeparam name="TValue">The value type.</typeparam>
        /// <param name="thisPtr">The link object.</param>
        /// <param name="key">The key.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>The property value or the default.</returns>
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

        /// <summary>
        /// Adds an object to a connection's extension.
        /// </summary>
        /// <param name="connection">The connection.</param>
        /// <param name="extension">The extension object.</param>
        public static void AddExtension(this AmqpConnection connection, object extension)
        {
            connection.Extensions.Add(extension.GetType(), extension);
        }

        /// <summary>
        /// Tries to get an object from a connection's extension.
        /// </summary>
        /// <typeparam name="T">The object type.</typeparam>
        /// <param name="connection">The connection.</param>
        /// <param name="extension">The extension object.</param>
        /// <returns>true if the object is found or false otherwise.</returns>
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

        /// <summary>
        /// Gets an array segment for a given buffer.
        /// </summary>
        /// <param name="buffer">The input buffer.</param>
        /// <returns>The array segment.</returns>
        public static ArraySegment<byte> AsSegment(this ByteBuffer buffer)
        {
            if (buffer == null)
            {
                return default(ArraySegment<byte>);
            }

            return new ArraySegment<byte>(buffer.Buffer, buffer.Offset, buffer.Length);
        }

        /// <summary>
        /// Finds an object of given type from the dictionary.
        /// </summary>
        /// <typeparam name="T">The type to find.</typeparam>
        /// <param name="extensions">The dictionary.</param>
        /// <returns>The object matching the type, or default(T) if not found.</returns>
        public static T Find<T>(this IDictionary<Type, object> extensions)
        {
            if (extensions.TryGetValue(typeof(T), out object value) &&
                value is T typedValue)
            {
                return typedValue;
            }

            return default;
        }

        internal static IAsyncResult ToAsyncResult<T>(this Task<T> task, AsyncCallback callback, object state)
        {
            var tcs = new TaskCompletionSource<T>(state);
            task.ContinueWith(_t =>
            {
                if (_t.IsFaulted)
                    tcs.TrySetException(_t.Exception.InnerExceptions);
                else if (_t.IsCanceled)
                    tcs.TrySetCanceled();
                else
                    tcs.TrySetResult(_t.Result);

                if (callback != null)
                    callback(tcs.Task);
            }, TaskScheduler.Default);

            return tcs.Task;
        }

        internal static T EndAsyncResult<T>(IAsyncResult asyncResult)
        {
            return ((Task<T>)asyncResult).Result;
        }
    }
}