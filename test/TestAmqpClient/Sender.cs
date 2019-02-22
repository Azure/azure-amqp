using System;
using System.Threading.Tasks;
using Microsoft.Azure.Amqp;
using Microsoft.Azure.Amqp.Framing;

namespace TestAmqpClient
{
    class Sender : Client<SendingAmqpLink>
    {
        public Sender(Options options)
            : base(options)
        {
        }

        protected override async Task ExecuteAsync()
        {
            int i = 0;
            while (this.Attempt())
            {
                string body = this.options.BodySize == 0 ?
                    "hello amqp" :
                    new string('A', this.options.BodySize);
                AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = body });
                ArraySegment<byte> tag = new ArraySegment<byte>(BitConverter.GetBytes(i++));
                Outcome outcome = await this.link.SendMessageAsync(message, tag, new ArraySegment<byte>(), this.link.DefaultOpenTimeout);
                if (outcome.DescriptorCode == Accepted.Code)
                {
                    this.Success();
                }
                else
                {
                    this.Failure();
                }
            }
        }

        protected override SendingAmqpLink CreateLink()
        {
            AmqpLinkSettings settings = new AmqpLinkSettings();
            settings.LinkName = $"sender-{DateTime.UtcNow.Ticks}";
            settings.Role = false;
            settings.Source = new Source(); ;
            settings.Target = new Target() { Address = this.options.Node }; ;
            settings.InitialDeliveryCount = 0;
            return new SendingAmqpLink(this.session, settings);
        }
    }
}