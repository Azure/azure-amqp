// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.PCL
{
    class Resources
    {
        public static string ReferenceAssemblyInvalidUse
        {
            get
            {
                return "AMQP reference assembly cannot be loaded at runtime.";
            }
        }
    }
}

namespace PlatformExtensions
{
    static class PlatformExtensions
    {
        // IsAssignableFrom is an extension method on other platform. So, avoid ambiguity, we protect this definition under a PCL block.
        public static bool IsAssignableFrom(this global::System.Type t, global::System.Type c)
        {
            throw new global::System.NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }
    }
}

namespace System.Net
{
    // A dummy implementation of EndPoint is okay to unblock compiling.
    // PCL profiles are not supported for this AMQP library yet, and if the code reaches this point, it should rightfully throw an exception.
    public class EndPoint
    {
        public EndPoint()
        {
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }
    }
}
