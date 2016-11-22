// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;

    abstract class ActionItem
    {
        public static void Schedule(WaitCallback callback, object state)
        {
            throw new NotImplementedException(Microsoft.Azure.Amqp.PCL.Resources.ReferenceAssemblyInvalidUse);
        }
    }
}

