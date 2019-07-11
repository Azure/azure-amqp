using System;
using System.Threading.Tasks;
using Microsoft.Azure.Amqp;
using Microsoft.Azure.Amqp.Framing;

namespace TestAmqpClient
{
    class Receiver : Client<ReceivingAmqpLink>
    {
        public Receiver(Options options)
            : base(options)
        {
        }

        protected override async Task ExecuteAsync()
        {
            while (true)
            {
                AmqpMessage message = await this.link.ReceiveMessageAsync();
                if (message != null)
                {
                    this.link.DisposeDelivery(message, true, new Accepted());
                    if (!this.Attempt())
                    {
                        break;
                    }
                }
            }
        }

        protected override ReceivingAmqpLink CreateLink()
        {
            AmqpLinkSettings settings = new AmqpLinkSettings();
            settings.LinkName = $"receiver-{DateTime.UtcNow.Ticks}";
            settings.Role = true;
            settings.Source = new Source() { Address = this.options.Node };
            settings.Target = new Target();
            settings.TotalLinkCredit = 300;
            return new ReceivingAmqpLink(this.session, settings);
        }
    }
}