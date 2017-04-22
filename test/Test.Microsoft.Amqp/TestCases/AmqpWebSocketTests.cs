namespace Test.Microsoft.Azure.Amqp
{
#if NET45
    using System;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Framing;
    using Xunit;
    using TestAmqpBroker;

    [Trait("Category", TestCategory.Current)]
    public class AmqpWebSocketTests
    {
        [Fact]
        public void AmqpWebSocketTransportTest()
        {
            string address = "ws://localhost:28088";
            var broker = new TestAmqpBroker(new string[] { address }, null, null, null);
            try
            {
                broker.Start();

                string queue = "AmqpWebSocketTransportTest";
                broker.AddQueue(queue);

                AmqpConnection connection = AmqpConnection.Factory.OpenConnectionAsync(address).GetAwaiter().GetResult();

                AmqpSession session = connection.CreateSession(new AmqpSessionSettings());
                session.Open();

                SendingAmqpLink sLink = new SendingAmqpLink(session, AmqpUtils.GetLinkSettings(true, queue, SettleMode.SettleOnSend));
                sLink.Open();

                int messageCount = 100;
                for (int i = 0; i < messageCount; i++)
                {
                    AmqpMessage message = AmqpMessage.Create(new AmqpValue() { Value = "message" + i });
                    sLink.SendMessageAsync(message, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary, TimeSpan.FromSeconds(10)).Wait();

                }

                sLink.Close();

                ReceivingAmqpLink rLink = new ReceivingAmqpLink(session, AmqpUtils.GetLinkSettings(false, queue, SettleMode.SettleOnReceive, 100));
                rLink.Open();

                for (int i = 0; i < messageCount; i++)
                {
                    AmqpMessage message2 = rLink.ReceiveMessageAsync(TimeSpan.FromSeconds(60)).GetAwaiter().GetResult();
                    Assert.NotNull(message2);

                    rLink.AcceptMessage(message2, false);
                    message2.Dispose();
                }

                rLink.Close();

                connection.Close();
            }
            finally
            {
                broker.Stop();
            }
        }
    }
#endif
}
