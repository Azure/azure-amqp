// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    static class TestCategory
    {
#if NETSTANDARD
        public const string Current = ".Net Core";
#elif WINDOWS_UWP
        public const string Current = "Windows UWP";
#else
        public const string Current = "Full .NET";
#endif
    }
}
