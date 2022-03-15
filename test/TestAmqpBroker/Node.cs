// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace TestAmqpBroker
{
    using System;
    using System.Collections.Concurrent;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Framing;

    public interface INode
    {
        string Name { get; }

        void OnAttachLink(AmqpLink link);
    }

    public abstract class RpcNode : INode
    {
        readonly string name;
        readonly ConcurrentDictionary<Address, SendingAmqpLink> senders;

        public RpcNode(string name)
        {
            this.name = name;
            this.senders = new ConcurrentDictionary<Address, SendingAmqpLink>(2, 4);
        }

        public string Name => this.name;

        public void OnAttachLink(AmqpLink link)
        {
            if (link.IsReceiver)
            {
                var receiver = (ReceivingAmqpLink)link;
                receiver.RegisterMessageListener(this.OnMessage);
                receiver.SetTotalLinkCredit(30u, true, true);
            }
            else
            {
                var sender = (SendingAmqpLink)link;
                Address replyTo = sender.Settings.Address(false);
                if (this.senders.TryAdd(replyTo, sender))
                {
                    sender.SafeAddClosed(this.OnLinkClose);
                }
                else
                {
                    sender.SafeClose(new AmqpException(AmqpErrorCode.NotAllowed,
                        $"A link with return target address '{replyTo}' already exists on node '{this.name}'"));
                }
            }
        }

        protected abstract Task<AmqpMessage> HandleRequestAsync(AmqpMessage request);

        void OnLinkClose(object sender, EventArgs args)
        {
            var link = (SendingAmqpLink)sender;
            Address replyTo = link.Settings.Address(false);
            this.senders.TryRemove(replyTo, out _);
        }

        async void OnMessage(AmqpMessage message)
        {
            using (message)
            {
                try
                {
                    Address reployTo = message.Properties?.ReplyTo;
                    if (reployTo == null || !this.senders.TryGetValue(reployTo, out SendingAmqpLink sender))
                    {
                        return;
                    }

                    AmqpMessage response = await this.HandleRequestAsync(message);
                    response.Properties.CorrelationId = message.Properties.MessageId;
                    response.Settled = true;
                    sender.SendMessageNoWait(response, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary);
                }
                catch (Exception)
                {
                }
            }
        }
    }

    public class CbsNode : RpcNode
    {
        public CbsNode() :
            base("$cbs")
        {
        }

        public TimeSpan ProcessingTime { get; set; }

        protected override async Task<AmqpMessage> HandleRequestAsync(AmqpMessage request)
        {
            if (this.ProcessingTime > TimeSpan.Zero)
            {
                await Task.Delay(this.ProcessingTime);
            }

            var response = AmqpMessage.Create();
            response.ApplicationProperties.Map["status-code"] = 200;
            return response;
        }
    }
}
