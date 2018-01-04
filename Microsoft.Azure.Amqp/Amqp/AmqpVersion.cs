// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Globalization;

    public struct AmqpVersion : IEquatable<AmqpVersion>
    {
        readonly byte major;
        readonly byte minor;
        readonly byte revision;

        public AmqpVersion(byte major, byte minor, byte revision)
        {
            this.major = major;
            this.minor = minor;
            this.revision = revision;
        }

        public AmqpVersion(Version version)
            : this((byte)version.Major, (byte)version.Minor, (byte)version.Revision)
        {
        }

        public byte Major
        {
            get { return this.major; }
        }

        public byte Minor
        {
            get { return this.minor; }
        }

        public byte Revision
        {
            get { return this.revision; }
        }

        public bool Equals(AmqpVersion other)
        {
            // Assume revision does not have breaking changes
            return this.Major == other.Major && this.Minor == other.Minor;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "{0}.{1}.{2}",
                this.Major,
                this.Minor,
                this.Revision);
        }
    }
}
