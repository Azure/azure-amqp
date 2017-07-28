using System;
using System.Threading.Tasks;

namespace TestAmqpClient
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.Out.WriteLine("TestAmqpClient.exe send|receive [options]");
                Options.Print(Console.Out);
                return 0;
            }

            try
            {
                RunAsync(args).GetAwaiter().GetResult();
                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(exception.ToString());
                return 1;
            }
        }

        static async Task RunAsync(string[] args)
        {
            IClient client;
            if (string.Equals("send", args[0], StringComparison.OrdinalIgnoreCase))
            {
                client = new Sender(Options.Parse(args, 1));
            }
            else if (string.Equals("receive", args[0], StringComparison.OrdinalIgnoreCase))
            {
                client = new Receiver(Options.Parse(args, 1));
            }
            else
            {
                throw new ArgumentException(args[0]);
            }

            bool cleanup = true;
            try
            {
                await client.InitAsync();
                await client.RunAsync();
                cleanup = false;
                await client.CleanUpAsync();
            }
            catch
            {
                if (cleanup)
                {
                    await client.CleanUpAsync();
                }

                throw;
            }
            finally
            {
                Console.Out.WriteLine($"Done. {client.Status}");
            }
        }
    }
}