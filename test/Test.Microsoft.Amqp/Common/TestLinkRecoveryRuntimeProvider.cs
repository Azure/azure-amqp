// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using global::Microsoft.Azure.Amqp;

    class TestLinkRecoveryRuntimeProvider : TestRuntimeProvider, ILinkRecoveryRuntimeProvider
    {
        internal TestLinkRecoveryRuntimeProvider(IAmqpLinkTerminusManager linkTerminusManager, IAmqpDeliveryStore unsettledDeliveryStore)
        {
            this.LinkTerminusManager = linkTerminusManager;
            this.UnsettledDeliveryStore = unsettledDeliveryStore;
        }

        public IAmqpLinkTerminusManager LinkTerminusManager { get; }

        public IAmqpDeliveryStore UnsettledDeliveryStore { get; }
    }
}
