// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Threading.Tasks;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// MSTest extensions for assertions not directly available in the framework.
    /// </summary>
    static class AssertExtensions
    {
        /// <summary>
        /// Verifies that the action throws an exception of type T or any derived type.
        /// Equivalent to xUnit's Assert.ThrowsAny.
        /// </summary>
        public static T ThrowsAny<T>(Action action) where T : Exception
        {
            try
            {
                action();
            }
            catch (T ex)
            {
                return ex;
            }

            throw new AssertFailedException($"Expected exception of type {typeof(T)} or derived, but no exception was thrown.");
        }

        /// <summary>
        /// Verifies that the async action throws an exception of type T or any derived type.
        /// Equivalent to xUnit's Assert.ThrowsAnyAsync.
        /// </summary>
        public static async Task<T> ThrowsAnyAsync<T>(Func<Task> action) where T : Exception
        {
            try
            {
                await action();
            }
            catch (T ex)
            {
                return ex;
            }

            throw new AssertFailedException($"Expected exception of type {typeof(T)} or derived, but no exception was thrown.");
        }
    }
}
