// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using System.Net.Sockets;
    using System.Threading;

    public sealed class TcpTransportListener : TransportListener
    {
        readonly WaitCallback acceptTransportLoop;
        readonly TcpTransportSettings transportSettings;
        Socket[] listenSockets;

        public TcpTransportListener(TcpTransportSettings transportSettings)
            : base("tcp-listener")
        {
            this.acceptTransportLoop = this.AcceptTransportLoop;
            this.transportSettings = transportSettings;
        }

        protected override bool CloseInternal()
        {
            this.CloseOrAbortListenSockets(false);
            return true;
        }

        protected override void AbortInternal()
        {
            this.CloseOrAbortListenSockets(true);
        }

        protected override void OnListen()
        {
            string listenHost = this.transportSettings.Host;
            Fx.Assert(listenHost != null, "Host cannot be null!");
            List<IPAddress> addresses = new List<IPAddress>();
            IPAddress ipAddress;

            // TODO: Fix this code to listen on Any address for FQDN pointing to the local host machine.
            if (listenHost.Equals(string.Empty))
            {
                addresses.AddRange(Dns.GetHostAddressesAsync(listenHost).Result);
            }
            else if (listenHost.Equals("localhost", StringComparison.OrdinalIgnoreCase) ||
                listenHost.Equals(Environment.GetEnvironmentVariable("COMPUTERNAME"), StringComparison.OrdinalIgnoreCase) ||
                listenHost.Equals(Dns.GetHostEntryAsync(string.Empty).Result.HostName, StringComparison.OrdinalIgnoreCase))
            {
                if (Socket.OSSupportsIPv4)
                {
                    addresses.Add(IPAddress.Any);
                }

                if (Socket.OSSupportsIPv6)
                {
                    addresses.Add(IPAddress.IPv6Any);
                }
            }
            else if (IPAddress.TryParse(listenHost, out ipAddress))
            {
                addresses.Add(ipAddress);
            }
            else
            {
                addresses.AddRange(Dns.GetHostAddressesAsync(this.transportSettings.Host).Result);
            }

            if (addresses.Count == 0)
            {
                throw new InvalidOperationException(AmqpResources.GetString(AmqpResources.AmqpNoValidAddressForHost, this.transportSettings.Host));
            }

            this.listenSockets = new Socket[addresses.Count];
            for (int i = 0; i < addresses.Count; ++i)
            {
                this.listenSockets[i] = new Socket(addresses[i].AddressFamily, SocketType.Stream, ProtocolType.Tcp) { NoDelay = true };
                this.listenSockets[i].Bind(new IPEndPoint(addresses[i], this.transportSettings.Port));
                this.listenSockets[i].Listen(this.transportSettings.TcpBacklog);

                for (int j = 0; j < this.transportSettings.ListenerAcceptorCount; ++j)
                {
                    SocketAsyncEventArgs listenEventArgs = new SocketAsyncEventArgs();
                    listenEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(this.OnAcceptComplete);
                    listenEventArgs.UserToken = this.listenSockets[i];
                    ActionItem.Schedule(this.acceptTransportLoop, listenEventArgs);
                }
            }
        }

        void CloseOrAbortListenSockets(bool abort)
        {
            if (this.listenSockets != null)
            {
                for (int i = 0; i < this.listenSockets.Length; ++i)
                {
                    Socket socket = this.listenSockets[i];
                    this.listenSockets[i] = null;
                    if (socket != null)
                    {
                        socket.Dispose();
                    }
                }
            }
        }

        void AcceptTransportLoop(object state)
        {
            SocketAsyncEventArgs args = (SocketAsyncEventArgs)state;
            Socket listenSocket = (Socket)args.UserToken;
            while (this.State != AmqpObjectState.End)
            {
                try
                {
                    args.AcceptSocket = null;
                    if (!listenSocket.AcceptAsync(args))
                    {
                        if (!this.HandleAcceptComplete(args, true))
                        {
                            // Stop on error
                            break;
                        }
                    }
                    else
                    {
                        // Accept is pending
                        break;
                    }
                }
                catch (SocketException socketException)
                {
                    bool shouldRetry = this.ShouldRetryAccept(socketException.SocketErrorCode);
                    if (!shouldRetry)
                    {
                        args.Dispose();
                        this.SafeClose(socketException);
                        break;
                    }
                }
                catch (Exception exception) when (!Fx.IsFatal(exception))
                {
                    AmqpTrace.Provider.AmqpListenSocketAcceptError(this, false, exception.Message);
                    args.Dispose();
                    this.SafeClose(exception);
                    break;
                }
            }
        }

        void OnAcceptComplete(object sender, SocketAsyncEventArgs e)
        {
            if (this.HandleAcceptComplete(e, false))
            {
                this.AcceptTransportLoop(e);
            }
        }

        bool HandleAcceptComplete(SocketAsyncEventArgs e, bool completedSynchronously)
        {
            if (e.SocketError == SocketError.Success)
            {
                TcpTransport transport = new TcpTransport(e.AcceptSocket, this.transportSettings);
                transport.Open();

                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.Transport = transport;
                args.CompletedSynchronously = completedSynchronously;
                this.OnTransportAccepted(args);
                return true;
            }
            else
            {
                bool shouldRetry = this.ShouldRetryAccept(e.SocketError);
                if (!shouldRetry)
                {
                    e.Dispose();
                    this.SafeClose(new SocketException((int)e.SocketError));
                }

                return shouldRetry;
            }
        }

        bool ShouldRetryAccept(SocketError error)
        {
            if (error == SocketError.OperationAborted)
            {
                // Close/Abort was called
                return false;
            }
            else
            {
                AmqpTrace.Provider.AmqpListenSocketAcceptError(this, true, error.ToString());
                return true;
            }
        }
    }
}
