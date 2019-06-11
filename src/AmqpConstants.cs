// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Globalization;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines the constants for the protocol stack.
    /// </summary>
    public static class AmqpConstants
    {
        /// <summary>
        /// Uri scheme for AMQP without TLS.
        /// </summary>
        public const string SchemeAmqp = "amqp";
        /// <summary>
        /// Uri scheme for AMQP over TLS.
        /// </summary>
        public const string SchemeAmqps = "amqps";

        /// <summary>
        /// Gets or sets the default timeout used for all APIs that have an overload
        /// with a timeout parameter. Default is 60 seconds.
        /// </summary>
        public static TimeSpan DefaultTimeout = TimeSpan.FromSeconds(60);

        internal const string Vendor = "com.microsoft";
        internal const string TimeSpanName = Vendor + ":timespan";
        internal const string UriName = Vendor + ":uri";
        internal const string DateTimeOffsetName = Vendor + ":datetime-offset";
        internal const string OpenErrorName = Vendor + ":open-error";
        internal static readonly AmqpSymbol IoEvent = "io-event";
        internal static readonly AmqpSymbol BatchedMessageFormat = Vendor + ":batched-message-format";
        internal static readonly AmqpSymbol HostName = "hostname";
        internal static readonly AmqpSymbol NetworkHost = "network-host";
        internal static readonly AmqpSymbol Port = "port";
        internal static readonly AmqpSymbol Address = "address";

        internal static readonly ArraySegment<byte> NullBinary = new ArraySegment<byte>();
        internal static readonly ArraySegment<byte> EmptyBinary = new ArraySegment<byte>(new byte[0]);

        internal static readonly AmqpVersion DefaultProtocolVersion = new AmqpVersion(1, 0, 0);
        internal static readonly DateTime StartOfEpoch = DateTime.Parse("1970-01-01T00:00:00.0000000Z", CultureInfo.InvariantCulture).ToUniversalTime();
        internal static readonly DateTime MaxAbsoluteExpiryTime = DateTime.MaxValue.ToUniversalTime() - TimeSpan.FromDays(1);

        internal static readonly Accepted AcceptedOutcome = new Accepted();
        internal static readonly Released ReleasedOutcome = new Released();
        internal static readonly Rejected RejectedOutcome = new Rejected();
        internal static readonly Rejected RejectedNotFoundOutcome = new Rejected { Error = new Error() { Condition = AmqpErrorCode.NotFound } };
        internal static readonly Received ReceivedOutcome = new Received();

        // 311(0x137) is the IANA code for Microsoft (http://www.iana.org/assignments/enterprise-numbers/enterprise-numbers)
        internal const uint AmqpBatchedMessageFormat = 0x80013700;
        internal const uint AmqpMessageFormat = 0;
        internal const int DefaultPort = 5672;
        internal const int DefaultSecurePort = 5671;
        internal const int ProtocolHeaderSize = 8;
        internal const int TransportBufferSize = 8 * 1024;
        internal const int MinMaxFrameSize = 512;
        internal const uint DefaultMaxFrameSize = 64 * 1024;
        internal const ushort DefaultMaxConcurrentChannels = 8 * 1024;
        internal const uint DefaultMaxLinkHandles = 256 * 1024;
        internal const uint DefaultHeartBeatInterval = 90000;
        internal const uint MinimumHeartBeatIntervalMs = 5 * 1000;
        internal const uint DefaultWindowSize = 5000;
        internal const uint DefaultLinkCredit = 1000;
        internal const uint DefaultNextTransferId = 1;
        internal const int DefaultDispositionTimeout = 20;
        internal const int SegmentSize = 512;
    }
}
