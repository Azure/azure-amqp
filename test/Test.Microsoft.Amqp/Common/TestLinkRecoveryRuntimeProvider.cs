// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using global::Microsoft.Azure.Amqp;

    class TestLinkRecoveryRuntimeProvider : TestRuntimeProvider, ILinkRecoveryRuntimeProvider
    {
        internal TestLinkRecoveryRuntimeProvider(IAmqpTerminusStore terminusStore)
        {
            this.TerminusStore = terminusStore;
        }

        public IAmqpTerminusStore TerminusStore { get; }        
    }
}
