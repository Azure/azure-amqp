// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Encoding
{
    using System;
    using System.Collections.Generic;

    public struct MapKey : IEquatable<MapKey>
    {
        object key;

        public MapKey(object key)
        {
            this.key = key;
        }

        public object Key
        {
            get { return this.key; }
        }

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

        public override int GetHashCode()
        {
            if (this.key == null)
            {
                return 0;
            }

            return this.key.GetHashCode();
        }

        public override bool Equals(object obj)
        {
            return (obj is MapKey) && this.Equals((MapKey)obj);
        }

        public override string ToString()
        {
            return key == null ? "<null>" : key.ToString();
        }
    }
}
