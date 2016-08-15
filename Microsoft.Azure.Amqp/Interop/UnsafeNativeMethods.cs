// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if !NETSTANDARD && !MONOANDROID && !PCL

namespace Microsoft.Azure.Amqp.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Eventing;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security;
    using System.Text;
    using System.Threading;
    using Microsoft.Win32.SafeHandles;

    [SuppressUnmanagedCodeSecurity]    
    static class UnsafeNativeMethods
    {
        public const string KERNEL32 = "kernel32.dll";

        [DllImport(KERNEL32)]
        [ResourceExposure(ResourceScope.None)]
        [SecurityCritical]
        internal static extern bool IsDebuggerPresent();
    }
}

#endif // !NETSTANDARD