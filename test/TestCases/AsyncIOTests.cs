namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Transport;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AsyncIOTests
    {
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
            var transport = new TestTransport() { ThrowOnWriteNumber = 2 };
            var ioHandler = new TestIoHandler();
            var writer = new AsyncIO.AsyncWriter(transport, int.MaxValue, int.MaxValue, ioHandler);

            writer.WriteBuffer(CreateBuffer(1));
            writer.WriteBuffer(CreateBuffer(2));
            writer.WriteBuffer(CreateBuffer(3));

            transport.CompleteWrite();

            Assert.AreSame(transport.WriteException, ioHandler.Exception);
            Assert.AreEqual(1, transport.BatchBufferLists.Count);
            Assert.AreEqual(0, transport.BatchBufferLists[0].Count);
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

            public Exception WriteException { get; }

            public Action<int> OnWrite { get; set; }

            public List<IList<ByteBuffer>> BatchBufferLists { get; }

            public TransportAsyncCallbackArgs PendingWrite { get; private set; }

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
                    args.BytesTransfered = args.Count;
                    return false;
                }

                Assert.IsNull(this.PendingWrite);
                this.PendingWrite = args;
                return true;
            }

            public override bool ReadAsync(TransportAsyncCallbackArgs args)
            {
                throw new NotSupportedException();
            }

            public void CompleteWrite()
            {
                TransportAsyncCallbackArgs args = this.PendingWrite;
                Assert.IsNotNull(args);
                this.PendingWrite = null;
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

            public ByteBuffer CreateBuffer(int frameSize)
            {
                throw new NotSupportedException();
            }

            public void OnReceiveBuffer(ByteBuffer buffer)
            {
                throw new NotSupportedException();
            }

            public void OnIoFault(Exception exception)
            {
                this.Exception = exception;
            }

            public void OnIoEvent(IoEvent ioEvent, long queueSize)
            {
            }
        }
    }
}
