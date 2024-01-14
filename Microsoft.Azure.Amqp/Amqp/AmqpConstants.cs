// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Constants for the protocol stack. Extended constants should be defined in
    /// ClientConstants.cs
    /// </summary>
    public static class AmqpConstants
    {
        public const string Apache = "apache.org";
        public const string Vendor = "com.microsoft";
        public const string SchemeAmqp = "amqp";
        public const string SchemeAmqps = "amqps";
        public const string TimeSpanName = Vendor + ":timespan";
        public const string UriName = Vendor + ":uri";
        public const string DateTimeOffsetName = Vendor + ":datetime-offset";
        public const string OpenErrorName = Vendor + ":open-error";
        public const string BadCommand = "BadCommand";
        public const string AddRule = "AddRule";
        public const string DeleteRule = "DeleteRule";
        public const string GetMessageSessions = "GetMessageSessions";
        public const string Publish = "Publish";
        public const string Consume = "Consume";
        public const string Dispose = "Dispose";
        public static readonly AmqpSymbol BatchedMessageFormat = Vendor + ":batched-message-format";
        public static readonly AmqpSymbol SimpleWebTokenPropertyName = Vendor + ":swt";
        public static readonly AmqpSymbol HostName = "hostname";
        public static readonly AmqpSymbol NetworkHost = "network-host";
        public static readonly AmqpSymbol Port = "port";
        public static readonly AmqpSymbol Address = "address";
        public static readonly AmqpSymbol PublisherId = "publisher-id";
        public static readonly AmqpSymbol IoEvent = "io-event";

        public static readonly ArraySegment<byte> NullBinary = new ArraySegment<byte>();
        public static readonly ArraySegment<byte> EmptyBinary = new ArraySegment<byte>(new byte[0]);

        public static readonly AmqpVersion DefaultProtocolVersion = new AmqpVersion(1, 0, 0);
        public static readonly DateTime StartOfEpoch = DateTime.Parse("1970-01-01T00:00:00.0000000Z", CultureInfo.InvariantCulture).ToUniversalTime();
        public static readonly DateTime MaxAbsoluteExpiryTime = DateTime.MaxValue.ToUniversalTime() - TimeSpan.FromDays(1);

        [SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Justification="Should be a constant")]
        public static readonly Accepted AcceptedOutcome = new Accepted();

        [SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Justification = "Should be a constant")]
        public static readonly Released ReleasedOutcome = new Released();

        [SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Justification = "Should be a constant")]
        public static readonly Rejected RejectedOutcome = new Rejected();

        [SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Justification = "Should be a constant")]
        public static readonly Rejected RejectedNotFoundOutcome = new Rejected { Error = new Error() { Condition = AmqpErrorCode.NotFound } };

        [SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Justification = "Should be a constant")]
        public static readonly Received ReceivedOutcome = new Received();

        // 311(0x137) is the IANA code for Microsoft (http://www.iana.org/assignments/enterprise-numbers/enterprise-numbers)
        public const uint AmqpBatchedMessageFormat = 0x80013700;
        public const uint AmqpMessageFormat = 0;
        public const int DefaultPort = 5672;
        public const int DefaultSecurePort = 5671;
        public const int ProtocolHeaderSize = 8;
        public const int TransportBufferSize = 8 * 1024;
        public const int MinMaxFrameSize = 512;
        public const uint DefaultMaxFrameSize = 64 * 1024;
        public const ushort DefaultMaxConcurrentChannels = 8 * 1024;
        public const uint DefaultMaxLinkHandles = 256 * 1024;
        public const uint DefaultHeartBeatInterval = 90000;
        public const uint MinimumHeartBeatIntervalMs = 5 * 1000;
        public static readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(60);
        public const uint DefaultWindowSize = 5000;
        public const uint DefaultLinkCredit = 1000;
        public const uint DefaultNextTransferId = 1;

        public const int SegmentSize = 512;
        public const byte AmqpFormat = 1;

        internal static readonly List<AmqpMessage> EmptyMessages = new List<AmqpMessage>(0);
    }
}