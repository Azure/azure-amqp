// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Globalization;

    /// <summary>
    /// Defines an AMQP version.
    /// </summary>
    public struct AmqpVersion : IEquatable<AmqpVersion>
    {
        /// <summary>
        /// The 1.0.0 version.
        /// </summary>
        public static AmqpVersion V100 = new AmqpVersion(1, 0, 0);

        readonly byte major;
        readonly byte minor;
        readonly byte revision;

        /// <summary>
        /// Initializes the version.
        /// </summary>
        /// <param name="major">Major protocol version.</param>
        /// <param name="minor">Minor protocol version.</param>
        /// <param name="revision">Protocol revision.</param>
        public AmqpVersion(byte major, byte minor, byte revision)
        {
            this.major = major;
            this.minor = minor;
            this.revision = revision;
        }

        /// <summary>
        /// Gets the major protocol version.
        /// </summary>
        public byte Major
        {
            get { return this.major; }
        }

        /// <summary>
        /// Gets the minor protocol version.
        /// </summary>
        public byte Minor
        {
            get { return this.minor; }
        }

        /// <summary>
        /// Gets the protocol revision.
        /// </summary>
        public byte Revision
        {
            get { return this.revision; }
        }

        /// <summary>
        /// Determines whether two protocol versions are equal.
        /// </summary>
        /// <param name="other">The version to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
        public bool Equals(AmqpVersion other)
        {
            // Assume revision does not have breaking changes
            return this.Major == other.Major && this.Minor == other.Minor;
        }

        /// <summary>
        /// Returns a string that represents the version.
        /// </summary>
        /// <returns>The string representation.</returns>
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
