// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// A delegate to carry out a work item.
    /// </summary>
    /// <typeparam name="T">Type of the work item.</typeparam>
    public interface IWorkDelegate<in T>
    {
        /// <summary>
        /// The method to process a work item.
        /// </summary>
        /// <param name="work">The work item.</param>
        /// <returns>true if the work is completed, or false if the work cannot be completed at this moment.</returns>
        bool Invoke(T work);
    }
}
