namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Net;
    using System.Threading;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AsyncIOTests
    {
        [TestMethod]
        public void FrameBufferReaderRejectsInvalidSizes()
        {
            foreach (uint size in new uint[] { 0, 1, 2, 3, 4, 5, 6, 7, 513, uint.MaxValue })
            {
                byte[] prefix = { (byte)(size >> 24), (byte)(size >> 16), (byte)(size >> 8), (byte)size };
                for (int synchronousReads = 0; synchronousReads < 4; synchronousReads++)
                {
                    var handler = new TestIoHandler();
                    var transport = ReadFrame(handler, new[] { new[] { prefix[0], prefix[1] }, new[] { prefix[2], prefix[3] } }, synchronousReads);

                    Assert.IsInstanceOfType(handler.Exception, typeof(AmqpException));
                    Assert.AreEqual(1, handler.FaultCount);
                    Assert.AreEqual(0, handler.BufferCount);
                    Assert.IsNull(handler.ReceivedFrame);
                    AssertReadReset(transport.LastRead);
                }
            }
        }

        [TestMethod]
        public void FrameBufferReaderContainsAllocationFailure()
        {
            for (int synchronousReads = 0; synchronousReads < 4; synchronousReads++)
            {
                var failure = new InvalidOperationException("allocation failed");
                var handler = new TestIoHandler() { AllocationException = failure };
                var transport = ReadFrame(handler, new[] { new byte[] { 0, 0 }, new byte[] { 0, 8 } }, synchronousReads);

                Assert.AreSame(failure, handler.Exception);
                Assert.AreEqual(1, handler.FaultCount);
                Assert.AreEqual(1, handler.BufferCount);
                AssertReadReset(transport.LastRead);
            }
        }

        [TestMethod]
        public void FrameBufferReaderPreservesCompleteAndPartialFrames()
        {
            foreach (int size in new[] { 8, 512 })
            {
                foreach (bool partialSize in new[] { false, true })
                {
                    foreach (bool partialBody in new[] { false, true })
                    {
                        byte[] frame = new byte[size];
                        frame[2] = (byte)(size >> 8);
                        frame[3] = (byte)size;
                        for (int i = 4; i < frame.Length; i++)
                        {
                            frame[i] = (byte)i;
                        }

                        var reads = new List<byte[]>();
                        AddReads(reads, frame, 0, 4, partialSize);
                        AddReads(reads, frame, 4, size - 4, partialBody);
                        for (int synchronousReads = 0; synchronousReads < (1 << reads.Count); synchronousReads++)
                        {
                            var handler = new TestIoHandler();
                            var transport = ReadFrame(handler, reads.ToArray(), synchronousReads);

                            CollectionAssert.AreEqual(frame, handler.ReceivedFrame);
                            Assert.AreEqual(1, handler.BufferCount);
                            Assert.AreEqual(0, handler.FaultCount);
                            Assert.IsNull(handler.ReadBuffer.Buffer);
                            AssertReadReset(transport.LastRead);
                        }
                    }
                }
            }
        }

        [TestMethod]
        public void FrameBufferReaderHandlesReadFailures()
        {
            foreach (bool body in new[] { false, true })
            {
                foreach (bool partial in new[] { false, true })
                {
                    for (int failureKind = 0; failureKind < 3; failureKind++)
                    {
                        var reads = new List<byte[]>();
                        if (body)
                        {
                            reads.Add(new byte[] { 0, 0, 0, 8 });
                        }

                        if (partial)
                        {
                            reads.Add(new byte[] { 0, 0 });
                        }

                        reads.Add(Array.Empty<byte>());
                        for (int synchronousReads = 0; synchronousReads < (1 << reads.Count); synchronousReads++)
                        {
                            var failure = failureKind == 1 ? null : new InvalidOperationException("read failed");
                            var handler = new TestIoHandler();
                            var transport = ReadFrame(handler, reads.ToArray(), synchronousReads, failure, failureKind == 2);

                            if (failure == null)
                            {
                                Assert.IsInstanceOfType(handler.Exception, typeof(IOException));
                            }
                            else
                            {
                                Assert.AreSame(failure, handler.Exception);
                            }

                            Assert.AreEqual(1, handler.FaultCount);
                            Assert.IsNull(handler.ReceivedFrame);
                            Assert.AreEqual(body ? 1 : 0, handler.BufferCount);
                            if (body)
                            {
                                Assert.IsNull(handler.ReadBuffer.Buffer);
                            }

                            AssertReadReset(transport.LastRead);
                        }
                    }
                }
            }
        }

        static TestTransport ReadFrame(TestIoHandler handler, byte[][] reads, int synchronousReads, Exception failure = null, bool throwOnRead = false)
        {
            var transport = new TestTransport()
            {
                Reads = reads,
                SynchronousReads = synchronousReads,
                ReadException = failure,
                ThrowOnRead = throwOnRead
            };
            new AsyncIO.FrameBufferReader(handler, transport, 512).ReadFrame();
            while (transport.PendingRead != null)
            {
                var args = transport.PendingRead;
                transport.PendingRead = null;
                args.CompletedCallback(args);
            }

            Assert.AreEqual(reads.Length, transport.ReadCount, "The reader must stop at terminal completion.");
            return transport;
        }

        static void AddReads(List<byte[]> reads, byte[] frame, int offset, int count, bool partial)
        {
            int firstCount = partial ? count / 2 : count;
            byte[] first = new byte[firstCount];
            Array.Copy(frame, offset, first, 0, firstCount);
            reads.Add(first);
            if (partial)
            {
                byte[] second = new byte[count - firstCount];
                Array.Copy(frame, offset + firstCount, second, 0, second.Length);
                reads.Add(second);
            }
        }

        static void AssertReadReset(TransportAsyncCallbackArgs args)
        {
            Assert.IsNull(args.ByteBuffer);
            Assert.IsNull(args.Buffer);
            Assert.IsNull(args.UserToken);
            Assert.IsNull(args.Exception);
            Assert.AreEqual(0, args.Offset);
            Assert.AreEqual(0, args.Count);
            Assert.AreEqual(0, args.BytesTransfered);
        }

        [TestMethod]
        public void AsyncWriterReusesBatchBufferListAfterAsyncWrites()
        {
            var transport = new TestTransport();
            var writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, new TestIoHandler());

            writer.WriteBuffer(CreateBuffer(1));
            writer.WriteBuffer(CreateBuffer(2));
            writer.WriteBuffer(CreateBuffer(3));

            transport.CompleteWrite();
            IList<ByteBuffer> firstBatch = transport.PendingWrite.ByteBufferList;
            Assert.AreEqual(2, firstBatch.Count);
            Assert.AreEqual(2, firstBatch[0].Length);
            Assert.AreEqual(3, firstBatch[1].Length);

            writer.WriteBuffer(CreateBuffer(4));
            writer.WriteBuffer(CreateBuffer(5));

            transport.CompleteWrite();
            IList<ByteBuffer> secondBatch = transport.PendingWrite.ByteBufferList;
            Assert.AreSame(firstBatch, secondBatch);
            Assert.AreEqual(2, secondBatch.Count);
            Assert.AreEqual(4, secondBatch[0].Length);
            Assert.AreEqual(5, secondBatch[1].Length);

            transport.CompleteWrite();
            Assert.AreEqual(0, secondBatch.Count);
        }

        [TestMethod]
        public void AsyncWriterReusesBatchBufferListAfterSynchronousWrites()
        {
            var transport = new TestTransport() { CompletesSynchronously = true };
            AsyncIO.AsyncWriter writer = null;
            transport.OnWrite = writeCount =>
            {
                if (writeCount == 1)
                {
                    writer.WriteBuffer(CreateBuffer(2));
                    writer.WriteBuffer(CreateBuffer(3));
                }
                else if (writeCount == 2)
                {
                    writer.WriteBuffer(CreateBuffer(4));
                    writer.WriteBuffer(CreateBuffer(5));
                }
            };

            writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, new TestIoHandler());
            writer.WriteBuffer(CreateBuffer(1));

            Assert.AreEqual(2, transport.BatchBufferLists.Count);
            Assert.AreSame(transport.BatchBufferLists[0], transport.BatchBufferLists[1]);
            Assert.AreEqual(0, transport.BatchBufferLists[0].Count);
        }

        [TestMethod]
        public void AsyncWriterClearsBatchBufferListWhenTransportThrows()
        {
            using (var faultReceived = new ManualResetEventSlim())
            {
                var transport = new TestTransport() { ThrowOnWriteNumber = 2 };
                var ioHandler = new TestIoHandler() { OnFault = _ => faultReceived.Set() };
                var writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, ioHandler);

                writer.WriteBuffer(CreateBuffer(1));
                writer.WriteBuffer(CreateBuffer(2));
                writer.WriteBuffer(CreateBuffer(3));

                transport.CompleteWrite();

                Assert.IsTrue(faultReceived.Wait(TimeSpan.FromSeconds(10)));
                Assert.AreSame(transport.WriteException, ioHandler.Exception);
                Assert.AreEqual(1, transport.BatchBufferLists.Count);
                Assert.AreEqual(0, transport.BatchBufferLists[0].Count);
            }
        }

        [TestMethod]
        public void AsyncWriterDiscardsBatchBufferListAfterLargeBurst()
        {
            var transport = new TestTransport();
            var writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, new TestIoHandler());

            writer.WriteBuffer(CreateBuffer(1));
            for (int i = 0; i < 100; i++)
            {
                writer.WriteBuffer(CreateBuffer(1));
            }

            transport.CompleteWrite();
            IList<ByteBuffer> largeBatch = transport.BatchBufferLists[0];
            Assert.AreEqual(100, largeBatch.Count);

            writer.WriteBuffer(CreateBuffer(2));
            writer.WriteBuffer(CreateBuffer(3));

            transport.CompleteWrite();
            Assert.AreEqual(2, transport.BatchBufferLists.Count);
            Assert.AreNotSame(largeBatch, transport.BatchBufferLists[1]);
        }

        [TestMethod]
        public void AsyncWriterDefersSynchronousTransportThrow()
        {
            AssertSynchronousWriteFaultIsDeferred(new TestTransport() { ThrowOnWriteNumber = 1 });
        }

        [TestMethod]
        public void AsyncWriterDefersSynchronousTransportError()
        {
            AssertSynchronousWriteFaultIsDeferred(new TestTransport() { CompletesSynchronously = true, FailOnWriteNumber = 1 });
        }

        [TestMethod]
        public void AsyncWriterReportsAsynchronousTransportErrorOnCallbackThread()
        {
            var transport = new TestTransport();
            int callbackThread = 0;
            var ioHandler = new TestIoHandler() { OnFault = _ => callbackThread = Thread.CurrentThread.ManagedThreadId };
            var writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, ioHandler);
            writer.WriteBuffer(CreateBuffer(1));

            transport.CompleteWrite(transport.WriteException);

            Assert.AreSame(transport.WriteException, ioHandler.Exception);
            Assert.AreEqual(Thread.CurrentThread.ManagedThreadId, callbackThread);
        }

        static void AssertSynchronousWriteFaultIsDeferred(TestTransport transport)
        {
            using (var faultReceived = new ManualResetEventSlim())
            {
                var callerLock = new object();
                bool faultRanUnderCallerLock = true;
                int faultCount = 0;
                var ioHandler = new TestIoHandler()
                {
                    OnFault = _ =>
                    {
                        faultRanUnderCallerLock = Monitor.IsEntered(callerLock);
                        Interlocked.Increment(ref faultCount);
                        faultReceived.Set();
                    }
                };
                var writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, ioHandler);

                lock (callerLock)
                {
                    writer.WriteBuffer(CreateBuffer(1));
                }

                Assert.IsTrue(faultReceived.Wait(TimeSpan.FromSeconds(10)));
                Assert.AreSame(transport.WriteException, ioHandler.Exception);
                Assert.AreEqual(1, faultCount);
                Assert.IsFalse(faultRanUnderCallerLock, "Fault handling must not run inline under the sender's locks.");
            }
        }

        static ByteBuffer CreateBuffer(int size)
        {
            return new ByteBuffer(new byte[size], 0, size);
        }

        sealed class TestTransport : TransportBase
        {
            int writeCount;

            public TestTransport()
                : base("test")
            {
                this.BatchBufferLists = new List<IList<ByteBuffer>>();
                this.WriteException = new InvalidOperationException("write failed");
            }

            internal override EndPoint Local => null;

            internal override EndPoint Remote => null;

            public override string LocalEndPoint => null;

            public override string RemoteEndPoint => null;

            public bool CompletesSynchronously { get; set; }

            public int ThrowOnWriteNumber { get; set; }

            public int FailOnWriteNumber { get; set; }

            public Exception WriteException { get; }

            public Action<int> OnWrite { get; set; }

            public List<IList<ByteBuffer>> BatchBufferLists { get; }

            public TransportAsyncCallbackArgs PendingWrite { get; private set; }

            public byte[][] Reads { get; set; }
            public int SynchronousReads { get; set; }
            public Exception ReadException { get; set; }
            public bool ThrowOnRead { get; set; }
            public int ReadCount { get; private set; }
            public TransportAsyncCallbackArgs PendingRead { get; set; }
            public TransportAsyncCallbackArgs LastRead { get; private set; }

            public override void SetMonitor(ITransportMonitor usageMeter)
            {
            }

            public override bool WriteAsync(TransportAsyncCallbackArgs args)
            {
                this.writeCount++;
                if (args.ByteBufferList != null)
                {
                    this.BatchBufferLists.Add(args.ByteBufferList);
                }

                this.OnWrite?.Invoke(this.writeCount);
                if (this.ThrowOnWriteNumber == this.writeCount)
                {
                    throw this.WriteException;
                }

                if (this.CompletesSynchronously)
                {
                    args.CompletedSynchronously = true;
                    args.Exception = this.FailOnWriteNumber == this.writeCount ? this.WriteException : null;
                    args.BytesTransfered = args.Count;
                    return false;
                }

                Assert.IsNull(this.PendingWrite);
                this.PendingWrite = args;
                return true;
            }

            public override bool ReadAsync(TransportAsyncCallbackArgs args)
            {
                this.LastRead = args;
                int index = this.ReadCount++;
                if (index >= this.Reads.Length)
                {
                    throw new InvalidOperationException("Unexpected extra read.");
                }

                if (index == this.Reads.Length - 1 && this.ThrowOnRead)
                {
                    throw this.ReadException;
                }

                Array.Copy(this.Reads[index], 0, args.Buffer, args.Offset, this.Reads[index].Length);
                args.BytesTransfered = this.Reads[index].Length;
                args.Exception = index == this.Reads.Length - 1 ? this.ReadException : null;
                args.CompletedSynchronously = (this.SynchronousReads & (1 << index)) != 0;
                if (args.CompletedSynchronously)
                {
                    return false;
                }

                this.PendingRead = args;
                return true;
            }

            public void CompleteWrite(Exception exception = null)
            {
                TransportAsyncCallbackArgs args = this.PendingWrite;
                Assert.IsNotNull(args);
                this.PendingWrite = null;
                args.Exception = exception;
                args.BytesTransfered = args.Count;
                args.CompletedSynchronously = false;
                args.CompletedCallback(args);
            }

            protected override bool CloseInternal()
            {
                return true;
            }

            protected override void AbortInternal()
            {
            }
        }

        sealed class TestIoHandler : IIoHandler
        {
            public Exception Exception { get; private set; }

            public Action<Exception> OnFault { get; set; }

            public Exception AllocationException { get; set; }
            public int BufferCount { get; private set; }
            public int FaultCount { get; private set; }
            public ByteBuffer ReadBuffer { get; private set; }
            public byte[] ReceivedFrame { get; private set; }

            public ByteBuffer CreateBuffer(int frameSize)
            {
                this.BufferCount++;
                if (this.AllocationException != null)
                {
                    throw this.AllocationException;
                }

                this.ReadBuffer = new ByteBuffer(frameSize, false);
                AmqpBitConverter.WriteUInt(this.ReadBuffer, (uint)frameSize);
                return this.ReadBuffer;
            }

            public void OnReceiveBuffer(ByteBuffer buffer)
            {
                this.ReceivedFrame = new byte[buffer.Length];
                Array.Copy(buffer.Buffer, buffer.Offset, this.ReceivedFrame, 0, buffer.Length);
            }

            public void OnIoFault(Exception exception)
            {
                this.Exception = exception;
                this.FaultCount++;
                this.OnFault?.Invoke(exception);
            }

            public void OnIoEvent(IoEvent ioEvent, long queueSize)
            {
            }
        }
    }
}
