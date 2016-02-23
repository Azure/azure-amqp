// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Security;

    static class PartialTrustHelpers
    {
        internal static bool ShouldFlowSecurityContext
        {
            [Fx.Tag.SecurityNote(Critical = "used in a security-sensitive decision")]
            [SecurityCritical]
            get
            {
#if DNXCORE
                return false;
#else

                if (AppDomain.CurrentDomain.IsHomogenous)
                {
                    return false;
                }

                return SecurityManager.CurrentThreadRequiresSecurityContextCapture();
#endif
            }
        }

        [Fx.Tag.SecurityNote(Critical = "used in a security-sensitive decision")]
        [SecurityCritical]
        internal static bool UnsafeIsInFullTrust()
        {
#if DNXCORE
            return true;
#else
            if (AppDomain.CurrentDomain.IsHomogenous)
            {
                return AppDomain.CurrentDomain.IsFullyTrusted;
            }
            else
            {
                return !SecurityManager.CurrentThreadRequiresSecurityContextCapture();
            }
#endif
        }

#if !DNXCORE
        [Fx.Tag.SecurityNote(Critical = "Captures security context with identity flow suppressed, " +
            "this requires satisfying a LinkDemand for infrastructure.")]
        [SecurityCritical]
        internal static SecurityContext CaptureSecurityContextNoIdentityFlow()
        {
            // capture the security context but never flow windows identity
            if (SecurityContext.IsWindowsIdentityFlowSuppressed())
            {
                return SecurityContext.Capture();
            }
            else
            {
                using (SecurityContext.SuppressFlowWindowsIdentity())
                {
                    return SecurityContext.Capture();
                }
            }
        }
#endif // !DNXCORE
    }
}
