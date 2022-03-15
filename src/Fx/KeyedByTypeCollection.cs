// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace System.Collections.Generic
{
    /// <summary>
    /// A collection of items keyed by type. For internal use only.
    /// </summary>
    public class KeyedByTypeCollection<TItem> : Dictionary<Type, TItem>
    {
        /// <summary>
        /// Finds an item of type t. For internal use only.
        /// </summary>
        public T Find<T>()
        {
            return this.TryGetValue(typeof(T), out TItem value) && value is T t ? t : default;
        }
    }
}
