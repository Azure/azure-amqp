// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Defines the key type used in <see cref="AmqpMap"/>.
    /// </summary>
    public readonly struct MapKey : IEquatable<MapKey>
    {
        readonly object key;

        /// <summary>
        /// Initializes the key from an object.
        /// </summary>
        /// <param name="key">The key object. It can be null.</param>
        public MapKey(object key)
        {
            this.key = key;
        }

        /// <summary>
        /// Gets the object key.
        /// </summary>
        public object Key
        {
            get { return this.key; }
        }

        /// <summary>
        /// Determines whether two keys are equal.
        /// </summary>
        /// <param name="other">The key to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
        public bool Equals(MapKey other)
        {
            if (this.key == null && other.key == null)
            {
                return true;
            }

            if (this.key == null || other.key == null)
            {
                return false;
            }

            return this.key.Equals(other.key);
        }

        /// <summary>
        /// Returns a hash code of the key.
        /// </summary>
        /// <returns>The hash code.</returns>
        public override int GetHashCode()
        {
            if (this.key == null)
            {
                return 0;
            }

            return this.key.GetHashCode();
        }

        /// <summary>
        /// Determines whether two objects are equal.
        /// </summary>
        /// <param name="obj">The object to compare.</param>
        /// <returns>True if they are equal; false otherwise.</returns>
        public override bool Equals(object obj)
        {
            return (obj is MapKey) && this.Equals((MapKey)obj);
        }

        /// <summary>
        /// Returns a string that represents the key.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            return key == null ? "<null>" : key.ToString();
        }
    }
}
