// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace TestAmqpBroker
{
    using System;
    using System.Collections.Generic;
    using System.Security.Authentication;

    class Program
    {
        static void Usage()
        {
            Console.WriteLine("AmqpTestBroker url [url] [ssl:protocols] [/creds:user:pwd] [/cert:ssl_cert] [/cbs] [/queues:q1;q2;...]");
            Console.WriteLine("  url    amqp|amqps://host[:port] (can be multiple)");
            Console.WriteLine("  ssl    ssl protocols, e.g. tls,tls11,tls12,tls13");
            Console.WriteLine("  creds  username:password");
            Console.WriteLine("  cert   ssl cert find value (thumbprint or subject), default to url.host");
            Console.WriteLine("  cbs    enables a test CBS node with no token validation");
            Console.WriteLine("  queues semicolon seperated queue names. If not specified, the broker implicitly");
            Console.WriteLine("         creates a new node for any non-existing address.");
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
            string sslProtocols = null;
            string sslValue = null;
            string[] queues = null;
            bool parseEndpoint = true;
            bool enableCbs = false;

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
                    else if (args[i].StartsWith("/ssl:", StringComparison.OrdinalIgnoreCase))
                    {
                        sslProtocols = args[i].Substring(5);
                    }
                    else if (args[i].Equals("/cbs", StringComparison.OrdinalIgnoreCase))
                    {
                        enableCbs = true;
                    }
                    else if (args[i].StartsWith("/queues:", StringComparison.OrdinalIgnoreCase))
                    {
                        queues = args[i].Substring(8).Split(';');
                    }
                    else if (args[i].StartsWith("/cert:", StringComparison.OrdinalIgnoreCase))
                    {
                        sslValue = args[i].Substring(6);
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
            if (sslProtocols != null)
            {
                string[] parts = sslProtocols.Split(',');
                SslProtocols protocols = SslProtocols.None;
                foreach (var p in parts)
                {
                    if (Enum.TryParse<SslProtocols>(p, ignoreCase: true, out SslProtocols value))
                    {
                        protocols |= value;
                    }
                    else
                    {
                        Console.WriteLine("Ignore unknown SSL protocol: {0}", p);
                    }
                }

                broker.EnabledSslProtocols = protocols;
            }

            if (enableCbs)
            {
                broker.AddNode(new CbsNode());
            }

            broker.Start();

            Console.WriteLine("Broker started. Press the enter key to exit...");
            Console.ReadLine();

            broker.Stop();
            Console.WriteLine("Broker stopped");
        }
    }
}
