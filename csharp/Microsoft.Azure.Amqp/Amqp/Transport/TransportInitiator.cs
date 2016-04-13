// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;

    public abstract class TransportInitiator
    {
        /// <summary>
        /// Returns true if connect is pending, the callback is invoked upon completion.
        /// Returns false if connect is completed synchronously, the callback is not invoked.
        /// </summary>
        public abstract bool ConnectAsync(TimeSpan timeout, TransportAsyncCallbackArgs callbackArgs);
    }
}
