// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
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
        public static readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(60);

        /// <summary>
        /// A string constant as the domain name for Microsoft extensions.
        /// </summary>
        public const string Vendor = "com.microsoft";
        /// <summary>
        /// A string constant as the domain name for Apache extensions.
        /// </summary>
        public const string Apache = "apache.org";
        /// <summary>A symbol constant for IO event.</summary>
        public static readonly AmqpSymbol IoEvent = "io-event";
        /// <summary>A symbol constant for batched message format.</summary>
        public static readonly AmqpSymbol BatchedMessageFormat = Vendor + ":batched-message-format";
        /// <summary>A symbol constant for hostname</summary>
        public static readonly AmqpSymbol HostName = "hostname";
        /// <summary>A symbol constant for network host.</summary>
        public static readonly AmqpSymbol NetworkHost = "network-host";
        /// <summary>A symbol constant for port number.</summary>
        public static readonly AmqpSymbol Port = "port";
        /// <summary>A symbol constant for 'address'</summary>
        public static readonly AmqpSymbol Address = "address";
        /// <summary>A symbol constant for publisher id.</summary>
        public static readonly AmqpSymbol PublisherId = "publisher-id";
        /// <summary>A symbol constant for simple web token property.</summary>
        public static readonly AmqpSymbol SimpleWebTokenPropertyName = Vendor + ":swt";

        /// <summary>
        /// Null binary.
        /// </summary>
        public static readonly ArraySegment<byte> NullBinary = new ArraySegment<byte>();
        /// <summary>
        /// Empty binary.
        /// </summary>
        public static readonly ArraySegment<byte> EmptyBinary = new ArraySegment<byte>(new byte[0]);

        /// <summary>The default AMQP version 1.0.0.</summary>
        public static readonly AmqpVersion DefaultProtocolVersion = new AmqpVersion(1, 0, 0);
        /// <summary>The start of epoch of an AMQP timestamp.</summary>
        public static readonly DateTime StartOfEpoch = DateTime.Parse("1970-01-01T00:00:00.0000000Z", CultureInfo.InvariantCulture).ToUniversalTime();
        /// <summary>
        /// The maximum message absolute expiry time. It is deprecated and should not be used.
        /// </summary>
        public static readonly DateTime MaxAbsoluteExpiryTime = DateTime.MaxValue.ToUniversalTime() - TimeSpan.FromDays(1);

        /// <summary>
        /// The accepted outcome constant.
        /// </summary>
        public static readonly Accepted AcceptedOutcome = new Accepted();
        /// <summary>
        /// The released outcome constant.
        /// </summary>
        public static readonly Released ReleasedOutcome = new Released();
        /// <summary>
        /// The rejected outcome constant without error.
        /// </summary>
        public static readonly Rejected RejectedOutcome = new Rejected();
        /// <summary>
        /// The rejected outcome constant with "amqp:not-found" error.
        /// </summary>
        public static readonly Rejected RejectedNotFoundOutcome = new Rejected { Error = new Error() { Condition = AmqpErrorCode.NotFound } };
        /// <summary>
        /// The received outcome constant.
        /// </summary>
        public static readonly Received ReceivedOutcome = new Received();

        // 311(0x137) is the IANA code for Microsoft (http://www.iana.org/assignments/enterprise-numbers/enterprise-numbers)
        /// <summary>
        /// AMQP batch format. Each <see cref="Data"/> section is a serialized message.
        /// </summary>
        public const uint AmqpBatchedMessageFormat = 0x80013700;
        /// <summary>
        /// The standard AMQP message format.
        /// </summary>
        public const uint AmqpMessageFormat = 0;
        /// <summary>
        /// The standard AMQP plain tcp port.
        /// </summary>
        public const int DefaultPort = 5672;
        /// <summary>
        /// The standard AMQP secure tcp port.
        /// </summary>
        public const int DefaultSecurePort = 5671;
        /// <summary>The size in bytes of an AMQP protocol header.</summary>
        public const int ProtocolHeaderSize = 8;
        /// <summary>
        /// The default transport buffer size.
        /// </summary>
        public const int TransportBufferSize = 8 * 1024;
        /// <summary>The minimum max-frame-size (<see cref="Open.MaxFrameSize"/>) an implementation must support.</summary>
        public const int MinMaxFrameSize = 512;
        /// <summary>
        /// The default maximum frame size used by the library.
        /// </summary>
        public const uint DefaultMaxFrameSize = 64 * 1024;

        /// <summary>The AMQP format code.</summary>
        public const byte AmqpFormat = 1;
        /// <summary>Operation name for adding a rule.</summary>
        public const string AddRule = "AddRule";
        /// <summary>Operation name for bad command.</summary>
        public const string BadCommand = "BadCommand";
        /// <summary>Operation name for consume.</summary>
        public const string Consume = "Consume";
        /// <summary>Operation name for deleting a rule.</summary>
        public const string DeleteRule = "DeleteRule";
        /// <summary>Operation name for dispose.</summary>
        public const string Dispose = "Dispose";
        /// <summary>Operation name for getting message sessions.</summary>
        public const string GetMessageSessions = "GetMessageSessions";
        /// <summary>Operation name for publish.</summary>
        public const string Publish = "Publish";

        /// <summary>The name for TimeSpan type.</summary>
        public const string TimeSpanName = Vendor + ":timespan";
        /// <summary>The name for Uri type.</summary>
        public const string UriName = Vendor + ":uri";
        /// <summary>The name for DateTimeOffset type.</summary>
        public const string DateTimeOffsetName = Vendor + ":datetime-offset";
        /// <summary>The name for open error.</summary>
        public const string OpenErrorName = Vendor + ":open-error";
        /// <summary>The default maximum number of concurrent channels.</summary>
        public const ushort DefaultMaxConcurrentChannels = 8 * 1024;
        /// <summary>The default maximum number of link handles.</summary>
        public const uint DefaultMaxLinkHandles = 256 * 1024;
        /// <summary>The default heartbeat interval in milliseconds.</summary>
        public const uint DefaultHeartBeatInterval = 90000;
        /// <summary>The minimum heartbeat interval in milliseconds.</summary>
        public const uint MinimumHeartBeatIntervalMs = 5 * 1000;
        /// <summary>The default window size.</summary>
        public const uint DefaultWindowSize = 5000;
        /// <summary>The default link credit.</summary>
        public const uint DefaultLinkCredit = 1000;
        /// <summary>The default next transfer id.</summary>
        public const uint DefaultNextTransferId = 1;
        /// <summary>The segment size in bytes.</summary>
        public const int SegmentSize = 512;

        internal static readonly List<AmqpMessage> EmptyMessages = new List<AmqpMessage>(0);
    }
}
