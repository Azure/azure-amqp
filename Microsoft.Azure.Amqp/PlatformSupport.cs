// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if WINDOWS_UWP
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Windows.Networking;
using Windows.Networking.Sockets;
#endif

#if DNXCORE

// This interface doesn't exist in DNXCORE50, define it manually
namespace System
{
    public interface ICloneable
    {
        object Clone();
    }
}


#if WINDOWS_UWP

namespace System.Threading
{
    //
    // Summary:
    //     Represents a callback method to be executed by a thread pool thread.
    //
    // Parameters:
    //   state:
    //     An object containing information to be used by the callback method.
    public delegate void WaitCallback(object state);
}

#endif


#if WINDOWS_UWP

class Win32
{
    [DllImport("kernel32.dll")]
    public static extern int GetCurrentProcessId();
}

#endif

#endif // DNXCORE

namespace Diagnostics
{
    static class CurrentProcess
    {
        public static int ID
        {
            get
            {
#if WINDOWS_UWP
                return Win32.GetCurrentProcessId();
#else
                return System.Diagnostics.Process.GetCurrentProcess().Id;
#endif
            }
        }
    }
}

