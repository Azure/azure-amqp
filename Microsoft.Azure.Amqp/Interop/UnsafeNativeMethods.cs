// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

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

        public const int ERROR_SUCCESS = 0;
        public const int ERROR_INVALID_HANDLE = 6;
        public const int ERROR_OUTOFMEMORY = 14;
        public const int ERROR_MORE_DATA = 234;
        public const int ERROR_ARITHMETIC_OVERFLOW = 534;
        public const int ERROR_NOT_ENOUGH_MEMORY = 8;
        public const int ERROR_OPERATION_ABORTED = 995;
        public const int ERROR_IO_PENDING = 997;
        public const int ERROR_NO_SYSTEM_RESOURCES = 1450;

        //[SuppressMessage(FxCop.Category.Interoperability, FxCop.Rule.MarkBooleanPInvokeArgumentsWithMarshalAs, Justification = "Opened as CSDMain #183080.")]
        //[SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage,
        //    Justification = "This PInvoke call has been reviewed")]
        [DllImport(KERNEL32, BestFitMapping = false, CharSet = CharSet.Auto)]
        [ResourceExposure(ResourceScope.Process)]
        [SecurityCritical]
        public static extern SafeWaitHandle CreateWaitableTimer(IntPtr mustBeZero, bool manualReset, string timerName);

        //[SuppressMessage(FxCop.Category.Interoperability, FxCop.Rule.MarkBooleanPInvokeArgumentsWithMarshalAs, Justification = "Opened as CSDMain #183080.")]
        //[SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage,
        //    Justification = "This PInvoke call has been reviewed")]
        [DllImport(KERNEL32, ExactSpelling = true)]
        [ResourceExposure(ResourceScope.None)]
        [SecurityCritical]
        public static extern bool SetWaitableTimer(SafeWaitHandle handle, ref long dueTime, int period, IntPtr mustBeZero, IntPtr mustBeZeroAlso, bool resume);

        //[SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage,
        //    Justification = "This PInvoke call has been reviewed")]
        [DllImport(KERNEL32, SetLastError = false)]
        [ResourceExposure(ResourceScope.None)]
        [SecurityCritical]
        public static extern uint GetSystemTimeAdjustment(
            [Out] out int adjustment,
            [Out] out uint increment,
            [Out] out uint adjustmentDisabled
            );

        ////[DllImport(KERNEL32, SetLastError = true)]
        ////[return: MarshalAs(UnmanagedType.Bool)]
        ////[ResourceExposure(ResourceScope.None)]
        ////[SecurityCritical]
        ////static extern bool GetComputerNameEx
        ////    (
        ////    [In] ComputerNameFormat nameType,
        ////    [In, Out, MarshalAs(UnmanagedType.LPTStr)] StringBuilder lpBuffer,
        ////    [In, Out] ref int size
        ////    );

        ////[SecurityCritical]
        ////internal static string GetComputerName(ComputerNameFormat nameType)
        ////{
        ////    int length = 0;
        ////    if (!GetComputerNameEx(nameType, null, ref length))
        ////    {
        ////        int error = Marshal.GetLastWin32Error();

        ////        if (error != ERROR_MORE_DATA)
        ////        {
        ////            throw Fx.Exception.AsError(new System.ComponentModel.Win32Exception(error));
        ////        }
        ////    }

        ////    if (length < 0)
        ////    {
        ////        Fx.AssertAndThrow("GetComputerName returned an invalid length: " + length);
        ////    }

        ////    StringBuilder stringBuilder = new StringBuilder(length);
        ////    if (!GetComputerNameEx(nameType, stringBuilder, ref length))
        ////    {
        ////        int error = Marshal.GetLastWin32Error();
        ////        throw Fx.Exception.AsError(new System.ComponentModel.Win32Exception(error));
        ////    }

        ////    return stringBuilder.ToString();
        ////}

        [DllImport(KERNEL32)]
        [ResourceExposure(ResourceScope.None)]
        [SecurityCritical]
        internal static extern bool IsDebuggerPresent();

        [StructLayout(LayoutKind.Sequential)]
        internal struct WSABuffer
        {
            public int length;
            public IntPtr buffer;
        }
    }
}
