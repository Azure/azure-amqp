﻿namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using System.Net.Sockets;
    using global::Microsoft.Azure.Amqp.Transport;
    using Xunit;
    using System.Net;
    using global::Microsoft.Azure.Amqp;

    [Trait("Category", TestCategory.Current)]
    public class AmqpTransportTests
    {
        const int TestBytes = 1024;
        const int Iterations = 2;
        const int TestMaxNumber = 9999;

        [Fact]
        public void TcpTransportTest()
        {
            const string localHost = "localhost";
            const int port = 30888;

            TransportTestContext serverContext = new TransportTestContext()
            {
                MaxNumber = TestMaxNumber,
                TransportSettings = AmqpUtils.GetTcpSettings(localHost, port, true)
            };

            TransportTestContext clientContext = new TransportTestContext()
            {
                MaxNumber = TestMaxNumber,
                TransportSettings = AmqpUtils.GetTcpSettings(localHost, port, false)
            };

            Thread listenerThread = new Thread(new ParameterizedThreadStart(ListenerThread));
            listenerThread.Start(serverContext);

            Thread initiatorThread = new Thread(new ParameterizedThreadStart(InitiatorThread));
            initiatorThread.Start(clientContext);

            listenerThread.Join();
            initiatorThread.Join();

            Debug.WriteLine("TCP transport test completed.");
            Assert.True(clientContext.Success);
            Assert.True(serverContext.Success);
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
                var task = initiator.ConnectTaskAsync(TimeSpan.FromSeconds(1));
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

        internal static TransportBase AcceptServerTransport(TransportSettings settings)
        {
            ManualResetEvent complete = new ManualResetEvent(false);
            int closed = 0;
            TransportBase transport = null;

            Action<TransportListener, TransportAsyncCallbackArgs> onTransport = (l, a) =>
            {
                if (a.Exception != null)
                {
                    Debug.WriteLine(a.Exception.Message);
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

            TransportListener listener = settings.CreateListener();
            Debug.WriteLine("Listeners are waiting for connections...");
            listener.Listen(onTransport);

            complete.WaitOne();
            complete.Dispose();

            transport.Closed += (s, a) =>
            {
                listener.Close();
                Debug.WriteLine("Listeners Closed.");
            };

            return transport;
        }

        internal static TransportBase EstablistClientTransport(TransportSettings settings)
        {
            ManualResetEvent complete = new ManualResetEvent(false);
            TransportBase transport = null;

            Action<TransportAsyncCallbackArgs> onTransport = (a) =>
            {
                if (a.Exception != null)
                {
                    Debug.WriteLine(a.Exception.Message);
                }
                else
                {
                    Debug.WriteLine("Initiator established a transport.");
                    transport = a.Transport;
                }

                complete.Set();
            };

            TransportInitiator initiator = settings.CreateInitiator();
            Debug.WriteLine("Initiator is connecting to the server...");
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.CompletedCallback = onTransport;
            initiator.ConnectAsync(TimeSpan.FromSeconds(10), args);

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
            public bool Success { get; set; }
            public TransportSettings TransportSettings { get; set; }
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
                this.testContext.Success = true;
                this.transport = AcceptServerTransport(testContext.TransportSettings);

                try
                {
                    int expect = 1;
                    while (expect > 0)
                    {
                        int num = this.Read();
                        if (num == 0)
                        {
                            if (expect < this.testContext.MaxNumber)
                            {
                                Debug.WriteLine(string.Format("Got eof before finishing all numbers (expect={0})", expect));
                                this.testContext.Success = false;
                            }

                            break;
                        }
                        else if (num != expect)
                        {
                            Debug.WriteLine(string.Format("Expect {0} but got {1}", expect, num));
                            this.testContext.Success = false;
                            break;
                        }

                        this.Write(expect * 2);
                        ++expect;
                    }
                }
                catch (Exception exception)
                {
                    Debug.WriteLine("Server got exception: " + exception.ToString());
                    this.testContext.Success = false;
                }

                this.transport.Close();
                Debug.WriteLine("Done server.");
            }

            public void RunClientTest(TransportTestContext testContext)
            {
                this.testContext = testContext;
                this.testContext.Success = true;
                this.transport = EstablistClientTransport(testContext.TransportSettings);

                try
                {
                    for (int i = 1; i < this.testContext.MaxNumber; ++i)
                    {
                        this.Write(i);
                        int num = this.Read();
                        if (num != i * 2)
                        {
                            Debug.WriteLine(string.Format("Wrote {0} but got {1}", i, num));
                            this.testContext.Success = false;
                            break;
                        }
                    }
                }
                catch(Exception exception)
                {
                    Debug.WriteLine("Client got exception: " + exception.ToString());
                    this.testContext.Success = false;
                }

                this.transport.Close();
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
