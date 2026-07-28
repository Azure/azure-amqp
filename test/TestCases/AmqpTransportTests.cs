namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Threading;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AmqpTransportTests
    {
        const int TestMaxNumber = 9999;

        [TestMethod]
        public void TcpTransportTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            this.RunTransportTest("TcpTransportTest", localHost, port, client, server);
        }

        [TestMethod]
        public void TcpTransportClientDynamicBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            client.SendBufferSize = client.ReceiveBufferSize = 0;
            this.RunTransportTest("TcpTransportClientDynamicBufferTest", localHost, port, client, server);
        }

        [TestMethod]
        public void TcpTransportClientFixedBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            client.SendBufferSize = client.ReceiveBufferSize = 16 * 1024;
            this.RunTransportTest("TcpTransportClientFixedBufferTest", localHost, port, client, server);
        }

        [TestMethod]
        public void TcpTransportServerDynamicBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            server.SendBufferSize = server.ReceiveBufferSize = 0;
            this.RunTransportTest("TcpTransportClientFixedBufferTest", localHost, port, client, server);
        }

        [TestMethod]
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

        [TestMethod]
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

        [TestMethod]
        public void TcpTransportServerFixedBufferTest()
        {
            const string localHost = "localhost";
            const int port = 30888;
            var client = AmqpUtils.GetTcpSettings(localHost, port, true);
            var server = AmqpUtils.GetTcpSettings(localHost, port, false);
            server.SendBufferSize = server.ReceiveBufferSize = 16 * 1024;
            this.RunTransportTest("TcpTransportServerFixedBufferTest", localHost, port, client, server);
        }

        [TestMethod]
        public void TcpTransportMultiBufferOnlyTest()
        {
            // Isolation check: a single multi-buffer send through the real transport.
            IPAddress address = IPAddress.Loopback;
            var listener = new TcpListener(address, 0);
            listener.Start();
            int port = ((IPEndPoint)listener.LocalEndpoint).Port;
            Socket serverSocket = null;
            var accepted = new ManualResetEventSlim(false);
            ThreadPool.QueueUserWorkItem(s => { serverSocket = ((TcpListener)s).AcceptSocket(); accepted.Set(); }, listener);
            var clientSocket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp) { NoDelay = true };
            clientSocket.Connect(address, port);
            accepted.Wait(TimeSpan.FromSeconds(5));
            listener.Stop();

            var settings = new TcpTransportSettings();
            settings.SendBufferSize = settings.ReceiveBufferSize = 16 * 1024;
            var transport = new TcpTransport(clientSocket, settings);

            var bbList = new List<ByteBuffer>
            {
                new ByteBuffer(new byte[] { 1, 2, 3 }, 0, 3),
                new ByteBuffer(new byte[] { 4, 5 }, 0, 2),
            };
            var args = new TransportAsyncCallbackArgs();
            var done = new ManualResetEventSlim(false);
            Exception error = null;
            args.CompletedCallback = a => { error = a.Exception; done.Set(); };
            args.SetBuffer(bbList);

            bool pending = transport.WriteAsync(args);
            if (pending) done.Wait(TimeSpan.FromSeconds(5));
            Console.WriteLine($"MultiOnly: pending={pending} transferred={args.BytesTransfered} error={error}");
            Assert.IsNull(error, error?.Message);
            Assert.AreEqual(5, args.BytesTransfered);

            try { clientSocket.Shutdown(SocketShutdown.Both); } catch { }
            byte[] buf = new byte[16];
            int total = 0;
            try { total = serverSocket.Receive(buf); } catch { }
            Assert.AreEqual(5, total);
            clientSocket.Dispose();
            serverSocket.Dispose();
            done.Dispose();
            accepted.Dispose();
        }

        [TestMethod]
        public void TcpTransportMultiBufferAdapterReuseTest()
        {
            // Drives the reusable write adapter through grow -> shrink -> reuse and
            // single/multi path switching on a real loopback socket. Verifies byte-for-byte
            // integrity of the whole stream and that the adapter retains no segment
            // references after Reset (full-array clear, including the grown tail). Data
            // integrity alone cannot catch retention because the active Count gates what
            // is sent, so the adapter's backing array is inspected directly.
            IPAddress address = IPAddress.Loopback;
            TcpListener listener = new TcpListener(address, 0);
            listener.Start();
            int port = ((IPEndPoint)listener.LocalEndpoint).Port;

            Socket serverSocket = null;
            var accepted = new ManualResetEventSlim(false);
            ThreadPool.QueueUserWorkItem(s =>
            {
                serverSocket = ((TcpListener)s).AcceptSocket();
                accepted.Set();
            }, listener);

            var clientSocket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp) { NoDelay = true };
            clientSocket.Connect(address, port);
            Assert.IsTrue(accepted.Wait(TimeSpan.FromSeconds(5)), "server did not accept the connection");
            Assert.IsNotNull(serverSocket);
            listener.Stop();

            var settings = new TcpTransportSettings();
            settings.SendBufferSize = settings.ReceiveBufferSize = 16 * 1024;
            var transport = new TcpTransport(clientSocket, settings);

            // Each inner array is one ByteBuffer (one AMQP frame). Counts are chosen to
            // force a grow (10) then a shrink (3) so the grown tail must be cleared,
            // then a reuse (2), with single-buffer sends mixed in.
            int[][] batches =
            {
                new[] { 1 },                                       // single-buffer path
                new[] { 2, 3 },                                    // multi, count 2
                new[] { 5, 7, 11, 13, 17, 19, 23, 29, 31, 37 },   // multi, count 10 (grow)
                new[] { 100, 101, 102 },                           // multi, count 3 (shrink -> tail clear)
                new[] { 9, 8 },                                    // multi, count 2 (reuse)
                new[] { 1 },                                       // single-buffer path again
            };

            var expected = new MemoryStream();
            var sends = new List<byte[][]>();
            byte next = 1;
            foreach (int[] sizes in batches)
            {
                var frame = new byte[sizes.Length][];
                for (int i = 0; i < sizes.Length; i++)
                {
                    byte[] b = new byte[sizes[i]];
                    for (int j = 0; j < b.Length; j++)
                    {
                        b[j] = next;
                        expected.WriteByte(next);
                        next++;
                    }

                    frame[i] = b;
                }

                sends.Add(frame);
            }

            byte[] expectedBytes = expected.ToArray();

            // Drain the server side to EOF concurrently with the writes.
            var received = new MemoryStream();
            var readerDone = new ManualResetEventSlim(false);
            ThreadPool.QueueUserWorkItem(_ =>
            {
                byte[] buf = new byte[8192];
                int n;
                try
                {
                    while ((n = serverSocket.Receive(buf, 0, buf.Length, SocketFlags.None)) > 0)
                    {
                        received.Write(buf, 0, n);
                    }
                }
                catch (SocketException)
                {
                    // transport close may race with the final receive; tolerated
                }

                readerDone.Set();
            });

            var args = new TransportAsyncCallbackArgs();
            var done = new ManualResetEventSlim(false);
            Exception writeError = null;
            args.CompletedCallback = a =>
            {
                writeError = a.Exception;
                done.Set();
            };

            try
            {
                foreach (byte[][] buffers in sends)
                {
                    writeError = null;
                    done.Reset();

                    int total = 0;
                    if (buffers.Length == 1)
                    {
                        total = buffers[0].Length;
                        args.SetBuffer(buffers[0], 0, buffers[0].Length);
                    }
                    else
                    {
                        var bbList = new List<ByteBuffer>(buffers.Length);
                        for (int i = 0; i < buffers.Length; i++)
                        {
                            bbList.Add(new ByteBuffer(buffers[i], 0, buffers[i].Length));
                            total += buffers[i].Length;
                        }

                        args.SetBuffer(bbList);
                    }

                    bool pending = transport.WriteAsync(args);
                    if (pending)
                    {
                        Assert.IsTrue(done.Wait(TimeSpan.FromSeconds(5)), "write did not complete in time");
                    }

                    Assert.IsNull(writeError, writeError?.Message);
                    Assert.AreEqual(total, args.BytesTransfered, "bytes transferred mismatch");
                    args.Reset();
                }

                // The adapter is detached inside HandleWriteComplete, so after the last
                // write it must not retain the source list.
                BindingFlags nonPublic = BindingFlags.Instance | BindingFlags.NonPublic;
                object sendArgs = typeof(TcpTransport).GetField("sendEventArgs", nonPublic).GetValue(transport);
                Assert.IsNotNull(sendArgs, "sendEventArgs field not found");
                object adapter = sendArgs.GetType().GetField("bufferListAdapter", nonPublic).GetValue(sendArgs);
                Assert.IsNotNull(adapter, "bufferListAdapter field not found");
                Assert.IsNull(adapter.GetType().GetField("source", nonPublic).GetValue(adapter),
                    "adapter retained the source list after reset");

                transport.Close();
                Assert.IsTrue(readerDone.Wait(TimeSpan.FromSeconds(5)), "server did not reach EOF");
                CollectionAssert.AreEqual(expectedBytes, received.ToArray());
            }
            finally
            {
                clientSocket.Dispose();
                serverSocket.Dispose();
                done.Dispose();
                accepted.Dispose();
                readerDone.Dispose();
            }
        }

        [TestMethod]
        public void ConnectTimeoutTest()
        {
            const int port = 30888;
            IPAddress address = IPAddress.Loopback;
            // Create a listener socket but do not listen on it
            var socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp) { NoDelay = true };
            socket.Bind(new IPEndPoint(address, port));

            try
            {
                var tcp = new TcpTransportSettings() { Host = "localhost", Port = port };
                var amqp = new AmqpSettings();
                amqp.TransportProviders.Add(new AmqpTransportProvider());
                var initiator = new AmqpTransportInitiator(amqp, tcp);
                var task = initiator.ConnectAsync(TimeSpan.FromSeconds(1));
                Assert.IsFalse(task.IsCompleted);

                Thread.Sleep(2000);
                Assert.IsTrue(task.IsFaulted);
                Assert.IsNotNull(task.Exception);

                var ex = task.Exception.GetBaseException() as SocketException;
                Assert.IsNotNull(ex);
                Assert.AreEqual(SocketError.TimedOut, (SocketError)ex.ErrorCode);
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
            Assert.IsTrue(context.Success, context.Exception?.ToString());
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
