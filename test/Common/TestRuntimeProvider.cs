// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Security.Principal;
    using System.Threading;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Sasl;
    using global::Microsoft.Azure.Amqp.Transport;

    class TestRuntimeProvider : IRuntimeProvider
    {
        public static readonly ArraySegment<byte> NullBinary = new ArraySegment<byte>();
        public static readonly ArraySegment<byte> EmptyBinary = new ArraySegment<byte>(new byte[0]);

        Queue<AmqpMessage> messages = new Queue<AmqpMessage>();

        public TestRuntimeProvider() { }

        public Func<AmqpSession, AmqpLinkSettings, AmqpLink> LinkFactory { get; set; }

        public AmqpConnection CreateConnection(TransportBase transport, ProtocolHeader protocolHeader, bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings)
        {
            connectionSettings.ContainerId = this.GetType().Name;
            return new AmqpConnection(transport, protocolHeader, false, amqpSettings, connectionSettings);
        }

        public AmqpSession CreateSession(AmqpConnection connection, AmqpSessionSettings settings)
        {
            return new AmqpSession(connection, settings, this);
        }

        public AmqpLink CreateLink(AmqpSession session, AmqpLinkSettings settings)
        {
            if (this.LinkFactory != null)
            {
                return this.LinkFactory(session, settings);
            }

            AmqpLink link;
            if (settings.Role.Value)
            {
                var receiver = new ReceivingAmqpLink(session, settings);
                receiver.RegisterMessageListener(m =>
                {
                    this.messages.Enqueue(m.Clone());
                    receiver.AcceptMessage(m, true, true);
                    m.Dispose();
                });
                link = receiver;
            }
            else
            {
                var sender = new SendingAmqpLink(session, settings);
                sender.RegisterCreditListener((credit, drain, tx) =>
                {
                    AmqpMessage message = this.messages.Dequeue();
                    message.DeliveryAnnotations.Map["x-opt-sequence-number"] = 1;
                    sender.SendMessageNoWait(message, EmptyBinary, NullBinary);
                });
                sender.RegisterDispositionListener(d =>
                {
                    sender.DisposeDelivery(d, true, d.State);
                });
                link = sender;
            }

            return link;
        }

        public IAsyncResult BeginOpenLink(AmqpLink link, TimeSpan timeout, AsyncCallback callback, object state)
        {
            var result = new CompletedAsyncResult(callback, state);
            callback(result);
            return result;
        }

        public void EndOpenLink(IAsyncResult result)
        {
        }
    }

    class TestSaslPlainAuthenticator : ISaslPlainAuthenticator
    {
        public Task<IPrincipal> AuthenticateAsync(string identity, string password)
        {
            if (identity != password)
            {
                throw new UnauthorizedAccessException();
            }

            IPrincipal principal = new GenericPrincipal(new GenericIdentity(identity), new string[] { "SEND", "RECV" });
            return Task.FromResult(principal);
        }
    }

    class CompletedAsyncResult : IAsyncResult
    {
        public CompletedAsyncResult(AsyncCallback callback, object state)
        {
            this.AsyncState = state;
        }

        public bool IsCompleted => true;

        public WaitHandle AsyncWaitHandle => throw new NotImplementedException();

        public object AsyncState { get; set; }

        public bool CompletedSynchronously => true;
    }
}
