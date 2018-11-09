// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transport
{
    using System;
    using System.Net;
    using System.Threading.Tasks;

    sealed class WebSocketTransportListener : TransportListener
    {
        readonly Uri uri;
        readonly HttpListener httpListener;
        bool closed;

        public WebSocketTransportListener(string address)
            : base(WebSocketTransportSettings.WebSockets)
        {
            this.uri = GetHttpUri(address);
            this.httpListener = new HttpListener();
            this.httpListener.Prefixes.Add(this.uri.AbsoluteUri);
        }

        protected override void OnListen()
        {
            this.httpListener.Start();
            var task = this.AcceptListenerContextLoop();
        }

        protected override bool CloseInternal()
        {
            this.closed = true;
            this.httpListener.Stop();
            this.httpListener.Close();
            return true;
        }

        protected override void AbortInternal()
        {
            this.closed = true;
            this.httpListener.Stop();
            this.httpListener.Close();
        }

        static Uri GetHttpUri(string address)
        {
            UriBuilder ub = new UriBuilder(address);

            if (string.Equals(ub.Scheme, WebSocketTransportSettings.WebSockets, StringComparison.OrdinalIgnoreCase))
            {
                ub.Scheme = "http";
                ub.Port = ub.Port > 0 ? ub.Port : WebSocketTransportSettings.WebSocketsPort;
            }
            else if (string.Equals(ub.Scheme, WebSocketTransportSettings.SecureWebSockets, StringComparison.OrdinalIgnoreCase))
            {
                ub.Scheme = "https";
                ub.Port = ub.Port > 0 ? ub.Port : WebSocketTransportSettings.SecureWebSocketsPort;
            }
            else
            {
                throw new NotSupportedException(ub.Scheme);
            }


            return ub.Uri;
        }

        async Task AcceptListenerContextLoop()
        {
            while (!this.closed)
            {
                try
                {
                    HttpListenerContext context = await this.httpListener.GetContextAsync().ConfigureAwait(false);

                    var task = this.HandleListenerContextAsync(context);
                }
                catch (Exception exception)
                {
                    AmqpTrace.Provider.AmqpHandleException(exception, "listen loop");
                }
            }
        }

        async Task HandleListenerContextAsync(HttpListenerContext context)
        {
            try
            {
                string subProtocol = null;
                string[] subProtocols = context.Request.Headers.GetValues("Sec-WebSocket-Protocol");
                for (int i = 0; i < subProtocols.Length; i++)
                {
                    if (subProtocols[i].Equals(WebSocketTransportSettings.WebSocketSubProtocol) ||
                        subProtocols[i].Equals("AMQPWSB10")     // defined by the previous draft
                        )
                    {
                        subProtocol = subProtocols[i];
                        break;
                    }
                }

                if (subProtocol == null)
                {
                    context.Response.StatusCode = 400;
                    context.Response.StatusDescription = "No supported subprotocol was found.";
                    context.Response.OutputStream.Dispose();
                    return;
                }

                var wsContext = await context.AcceptWebSocketAsync(subProtocol).ConfigureAwait(false);
                var transport = new WebSocketTransport(wsContext.WebSocket, this.uri);

                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.Transport = transport;
                args.CompletedSynchronously = false;
                this.OnTransportAccepted(args);
            }
            catch (Exception exception)
            {
                AmqpTrace.Provider.AmqpHandleException(exception, "listen loop");

                context.Response.StatusCode = 500;
                context.Response.OutputStream.Dispose();
            }
        }
    }
}