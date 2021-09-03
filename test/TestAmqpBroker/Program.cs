// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace TestAmqpBroker
{
    using System;
    using System.Collections.Generic;

    class Program
    {
        static void Usage()
        {
            Console.WriteLine("AmqpTestBroker url [url] [/creds:user:pwd] [/cert:ssl_cert] [/queues:q1;q2;...]");
            Console.WriteLine("  url=amqp|amqps://host[:port] (can be multiple)");
            Console.WriteLine("  creds=username:passwrod");
            Console.WriteLine("  cert=ssl cert find value (thumbprint or subject), default to url.host");
            Console.WriteLine("  queues: semicolon seperated queue names. If not specified, the broker implicitly");
            Console.WriteLine("          creates a new node for any non-existing address.");
        }

        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                Usage();
            }
            else
            {
                try
                {
                    Run(args);
                }
                catch (Exception exception)
                {
                    Console.WriteLine(exception.ToString());
                }
            }
        }

        static void Run(string[] args)
        {
            List<string> endpoints = new List<string>();
            string creds = null;
            string sslValue = null;
            string[] queues = null;
            bool parseEndpoint = true;
            bool headless = false;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i][0] != '/' && parseEndpoint)
                {
                    endpoints.Add(args[i]);
                }
                else
                {
                    parseEndpoint = false;
                    if (args[i].StartsWith("/creds:", StringComparison.OrdinalIgnoreCase))
                    {
                        creds = args[i].Substring(7);
                    }
                    else if (args[i].StartsWith("/queues:", StringComparison.OrdinalIgnoreCase))
                    {
                        queues = args[i].Substring(8).Split(';');
                    }
                    else if (args[i].StartsWith("/cert:", StringComparison.OrdinalIgnoreCase))
                    {
                        sslValue = args[i].Substring(6);
                    }
                    else if (args[i].Equals("/headless", StringComparison.OrdinalIgnoreCase))
                    {
                        headless = true;
                    }
                    else
                    {
                        Console.WriteLine("Unknown argument: {0}", args[i]);
                        Usage();
                        return;
                    }
                }
            }

            var broker = new TestAmqpBroker(endpoints, creds, sslValue, queues);
            broker.Start();

            Console.Write("Broker started.");
            if (headless)
            {
                Console.WriteLine();
                new System.Threading.AutoResetEvent(false).WaitOne();
            }
            Console.WriteLine(" Press the enter key to exit...");
            Console.ReadLine();

            broker.Stop();
            Console.WriteLine("Broker stopped");
        }
    }
}
