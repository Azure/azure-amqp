namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Threading;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Transport;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class AmqpTransportTests
    {
        const int TestMaxNumber = 9999;

        [Fact]
        public void TcpTransportTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            this.RunTransportTest("TcpTransportTest", localHost, port, client, server);
        }

        [Fact]
        public void TcpTransportClientDynamicBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            client.SendBufferSize = client.ReceiveBufferSize = 0;
            this.RunTransportTest("TcpTransportClientDynamicBufferTest", localHost, port, client, server);
        }

        [Fact]
        public void TcpTransportClientFixedBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            client.SendBufferSize = client.ReceiveBufferSize = 16 * 1024;
            this.RunTransportTest("TcpTransportClientFixedBufferTest", localHost, port, client, server);
        }

        [Fact]
        public void TcpTransportServerDynamicBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            server.SendBufferSize = server.ReceiveBufferSize = 0;
            this.RunTransportTest("TcpTransportClientFixedBufferTest", localHost, port, client, server);
        }

        [Fact]
        public void TcpTransportServerPooledBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            ByteBuffer.InitTransportBufferManager(4 * 1024, 200);
            try
            {
                var client = AmqpUtils.GetTcpSettings(localHost, port, true);
                var server = AmqpUtils.GetTcpSettings(localHost, port, false);
                this.RunTransportTest("TcpTransportServerPooledBufferTest", localHost, port, client, server);
            }
            finally
            {
                typeof(ByteBuffer).GetField("TransportBufferManager", BindingFlags.Static | BindingFlags.NonPublic).SetValue(null, null);
            }
        }

        [Fact]
        public void TcpTransportServerDynamicPooledBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            ByteBuffer.InitTransportBufferManager(4 * 1024, 200);
            try
            {
                var client = AmqpUtils.GetTcpSettings(localHost, port, true);
                var server = AmqpUtils.GetTcpSettings(localHost, port, false);
                server.SendBufferSize = server.ReceiveBufferSize = 0;
                this.RunTransportTest("TcpTransportServerDynamicPooledBufferTest", localHost, port, client, server);
            }
            finally
            {
                typeof(ByteBuffer).GetField("TransportBufferManager", BindingFlags.Static | BindingFlags.NonPublic).SetValue(null, null);
            }
        }

        [Fact]
        public void TcpTransportServerFixedBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            server.SendBufferSize = server.ReceiveBufferSize = 16 * 1024;
            this.RunTransportTest("TcpTransportServerFixedBufferTest", localHost, port, client, server);
        }

        [Fact]
        public void ConnectTimeoutTest()
        {
            const int port = 30888;
            IPAddress address = IPAddress.Loopback;
            // Creat a listener socket but do not listen on it
            var socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp) { NoDelay = true };
            socket.Bind(new IPEndPoint(address, port));

            try
            {
                var tcp = new TcpTransportSettings() { Host = "localhost", Port = port };
                var amqp = new AmqpSettings();
                amqp.TransportProviders.Add(new AmqpTransportProvider());
                var initiator = new AmqpTransportInitiator(amqp, tcp);
                var task = initiator.ConnectAsync(TimeSpan.FromSeconds(1));
                Assert.False(task.IsCompleted);

                Thread.Sleep(2000);
                Assert.True(task.IsFaulted);
                Assert.NotNull(task.Exception);

                var ex = task.Exception.GetBaseException() as SocketException;
                Assert.NotNull(ex);
                Assert.Equal(SocketError.TimedOut, (SocketError)ex.ErrorCode);
            }
            finally
            {
                socket.Close();
            }
        }

        void RunTransportTest(string test, string host, int port, TransportSettings client, TransportSettings server)
        {
            Debug.WriteLine($"Test '{test}' start.");

            TransportTestContext context = new TransportTestContext()
            {
                MaxNumber = TestMaxNumber,
                Client = client,
                Server = server,
                ServerReady = new ManualResetEvent(false),
            };

            Thread listenerThread = new Thread(new ParameterizedThreadStart(ListenerThread));
            listenerThread.Start(context);

            Thread initiatorThread = new Thread(new ParameterizedThreadStart(InitiatorThread));
            initiatorThread.Start(context);

            listenerThread.Join();
            initiatorThread.Join();

            Debug.WriteLine($"Test '{test}' end.");
            Assert.True(context.Success, context.Exception?.ToString());
        }

        static TransportBase AcceptServerTransport(TransportTestContext testContext)
        {
            ManualResetEvent complete = new ManualResetEvent(false);
            int closed = 0;
            TransportBase transport = null;

            Action<TransportListener, TransportAsyncCallbackArgs> onTransport = (l, a) =>
            {
                if (a.Exception != null)
                {
                    Debug.WriteLine(a.Exception.Message);
                    testContext.Exception = a.Exception;
                }
                else
                {
                    Debug.WriteLine("Listener accepted a transport.");
                    transport = a.Transport;
                }

                if (Interlocked.Exchange(ref closed, 1) == 0)
                {
                    complete.Set();
                }
            };

            TransportListener listener = testContext.Server.CreateListener();
            listener.Listen(onTransport);
            testContext.ServerReady.Set();
            Debug.WriteLine("Listeners are waiting for connections...");

            complete.WaitOne();
            complete.Dispose();

            listener.Close();
            Debug.WriteLine("Listeners Closed.");

            return transport;
        }

        static TransportBase EstablistClientTransport(TransportTestContext testContext)
        {
            testContext.ServerReady.WaitOne();
            testContext.ServerReady.Dispose();

            ManualResetEvent complete = new ManualResetEvent(false);
            TransportBase transport = null;

            Action<TransportAsyncCallbackArgs> onTransport = (a) =>
            {
                if (a.Exception != null)
                {
                    Debug.WriteLine(a.Exception.Message);
                    testContext.Exception = a.Exception;
                }
                else
                {
                    Debug.WriteLine("Initiator established a transport.");
                    testContext.Exception = null;
                    transport = a.Transport;
                }

                complete.Set();
            };

            TransportInitiator initiator = testContext.Client.CreateInitiator();
            Debug.WriteLine("Initiator is connecting to the server...");
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.CompletedCallback = onTransport;
            if (!initiator.ConnectAsync(TimeSpan.FromSeconds(6), args))
            {
                onTransport(args);
            }

            complete.WaitOne();
            complete.Dispose();

            return transport;
        }

        static void ListenerThread(object state)
        {
            new TransportTestHelper().RunServerTest((TransportTestContext)state);
            Debug.WriteLine("ListenerThread done.");
        }

        static void InitiatorThread(object state)
        {
            new TransportTestHelper().RunClientTest((TransportTestContext)state);
            Debug.WriteLine("InitiatorThread done.");
        }

        class TransportTestContext
        {
            public int MaxNumber { get; set; }
            public Exception Exception { get; set; }
            public bool Success { get { return Exception == null; } }
            public TransportSettings Client { get; set; }
            public TransportSettings Server { get; set; }
            public ManualResetEvent ServerReady { get; set; }
        }

        class TransportTestHelper
        {
            TransportBase transport;
            TransportTestContext testContext;

            public TransportTestHelper()
            {
            }

            public void RunServerTest(TransportTestContext testContext)
            {
                this.testContext = testContext;

                try
                {
                    this.transport = AcceptServerTransport(testContext);
                    if (!testContext.Success)
                    {
                        return;
                    }

                    int expect = 1;
                    while (expect > 0)
                    {
                        int num = this.Read();
                        if (num == 0)
                        {
                            if (expect < this.testContext.MaxNumber)
                            {
                                this.testContext.Exception = new Exception(string.Format("Got eof before finishing all numbers (expect={0})", expect));
                            }

                            break;
                        }
                        else if (num != expect)
                        {
                            this.testContext.Exception = new Exception(string.Format("Expect {0} but got {1}", expect, num));
                            break;
                        }

                        this.Write(expect * 2);
                        ++expect;
                    }
                }
                catch (Exception exception)
                {
                    this.testContext.Exception = exception;
                }

                this.transport?.Close();
                Debug.WriteLine("Done server.");
            }

            public void RunClientTest(TransportTestContext testContext)
            {
                this.testContext = testContext;

                try
                {
                    this.transport = EstablistClientTransport(testContext);
                    if (!testContext.Success)
                    {
                        return;
                    }

                    for (int i = 1; i < this.testContext.MaxNumber; ++i)
                    {
                        this.Write(i);
                        int num = this.Read();
                        if (num != i * 2)
                        {
                            this.testContext.Exception = new Exception(string.Format("Wrote {0} but got {1}", i, num));
                            break;
                        }
                    }
                }
                catch (Exception exception)
                {
                    this.testContext.Exception = exception;
                }

                this.transport?.Close();
                Debug.WriteLine("Done client.");
            }

            void Write(int number)
            {
                byte[] buffer = BitConverter.GetBytes(number);
                ManualResetEvent writeComplete = new ManualResetEvent(false);

                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.SetBuffer(buffer, 0, buffer.Length);
                args.CompletedCallback = (o) => { writeComplete.Set(); };
                this.transport.WriteAsync(args);
                if (!args.CompletedSynchronously)
                {
                    writeComplete.WaitOne();
                }

                if (args.Exception != null)
                {
                    throw args.Exception;
                }
            }

            int Read()
            {
                ManualResetEvent readComplete = new ManualResetEvent(false);

                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                byte[] buffer = new byte[4];
                args.SetBuffer(buffer, 0, buffer.Length);
                args.CompletedCallback = (o) => { readComplete.Set(); };
                this.transport.ReadAsync(args);
                if (!args.CompletedSynchronously)
                {
                    readComplete.WaitOne();
                }

                if (args.Exception != null)
                {
                    throw args.Exception;
                }

                if (args.BytesTransfered == 0)
                {
                    Debug.WriteLine("Read got eof.");
                    return 0;
                }
                else
                {
                    return BitConverter.ToInt32(buffer, 0);
                }
            }
        }
    }
}
