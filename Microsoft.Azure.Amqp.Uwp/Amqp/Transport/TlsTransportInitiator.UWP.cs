// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using Windows.Networking.Sockets;

    sealed class TlsTransportInitiator : TcpTransportInitiator
    {
        readonly TlsTransportSettings transportSettings;

        public TlsTransportInitiator(TlsTransportSettings transportSettings)
            : base((TcpTransportSettings)transportSettings.InnerTransportSettings)
        {
            this.transportSettings = transportSettings;
            this.ProtectionLevel = SocketProtectionLevel.Tls12;
        }

        public override string ToString()
        {
            return "tls-initiator";
        }
    }
}
