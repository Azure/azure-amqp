namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using Xunit;

    [Trait("Category", TestCategory.Current)]
    public class UtilityTests
    {
        [Fact]
        public void SerializedWorkerTest()
        {
            const int workerCount = 3;
            const int workLoad = 1000000;
            const int totalCount = workLoad * workerCount;
            int completedCount = 0;
            int callCount = 0;
            int working = 0;
            ManualResetEvent completeEvent = new ManualResetEvent(false);

            WaitCallback callContinue = null;

            Func<object, bool> func = (o) =>
            {
                // this function should be always called single threaded
                if (Interlocked.Exchange(ref working, 1) == 1)
                {
                    Debug.WriteLine("Should not be called while working");
                    completeEvent.Set();
                    return false;
                }

                bool workCompleted = callCount++ % 30000 != 0;
                if (workCompleted)
                {
                    if (++completedCount == totalCount)
                    {
                        completeEvent.Set();
                    }
                }

                if (!workCompleted)
                {
                    ActionItem.Schedule(callContinue, null);
                }

                Interlocked.Exchange(ref working, 0);
                return workCompleted;
            };

            ParameterizedThreadStart producer = (o) =>
            {
                SerializedWorker<object> worker = (SerializedWorker<object>)o;
                for (int i = 0; i < workLoad; ++i)
                {
                    worker.DoWork(o);
                }
            };

            SerializedWorker<object> serialziedWorker = new SerializedWorker<object>(new Worker<object>(func));
            callContinue = (o) => { serialziedWorker.ContinueWork(); };

            Thread[] workerThreads = new Thread[workerCount];
            for (int i = 0; i < workerThreads.Length; ++i)
            {
                workerThreads[i] = new Thread(producer);
                workerThreads[i].Start(serialziedWorker);
            }

            // issue some continue work signal
            for (int i = 0; i < 20000; ++i)
            {
                serialziedWorker.ContinueWork();
            }

            // wait for all producer to finish
            for (int i = 0; i < workerThreads.Length; ++i)
            {
                workerThreads[i].Join();
            }

            bool waitOne = completeEvent.WaitOne(30 * 1000);
            Debug.WriteLine(string.Format("total: {0}, completed: {1}", totalCount, completedCount));
            if (!waitOne)
            {
                Assert.True(false, "Worker did not complete in time");
            }
            else
            {
                Assert.True(totalCount == completedCount, "Completed count is not correct.");
            }
        }

        [Fact]
        public void SerializedWorkerRaceTest()
        {
            int callCount = 0;

            Func<object, bool> func = (o) =>
            {
                if (callCount == 0)
                {
                    // call continue but return not completed
                    // to see if the worker can handle this race
                    SerializedWorker<object> worker = (SerializedWorker<object>)o;
                    worker.ContinueWork();
                    ++callCount;
                    return false;
                }
                else
                {
                    callCount = 100;
                    return true;
                }
            };

            SerializedWorker<object> serialziedWorker = new SerializedWorker<object>(new Worker<object>(func));
            serialziedWorker.DoWork(serialziedWorker);
            Assert.True(callCount == 100, "the work is not done even if continue is called.");
        }

        [Fact]
        public void SequenceNumberTest()
        {
            SequenceNumber sn0 = 0u;
            SequenceNumber sn1 = 10;
            SequenceNumber sn2 = (1u << 31) + 1;
            SequenceNumber sn3 = new SequenceNumber(uint.MaxValue - 2);
            SequenceNumber sn4 = 222;
            SequenceNumber sn5 = 2147483647u;
            SequenceNumber sn6 = 2147483648u;
            SequenceNumber sn7 = 0xFFFFFFF8u;
            SequenceNumber sn8 = 0xFFFFFFFFu;

            Assert.True(sn1 == 10u, "sn1 should be 10");
            Assert.False(sn1 == sn3, "sn1 != sn3");
            Assert.True(sn1 != sn5, "sn1 != sn5");
            Assert.False(sn1 != 10u, "sn1 == 10u");
            Assert.True(sn1 >= 10u, "sn1 should not be less than 10");
            Assert.True(sn1 < sn2, "10 should be less than 2 ^ 31 + 1");
            Assert.True(sn1 <= sn2, "10 should not be greater than 2 ^ 31 + 1");
            Assert.True(sn1 > sn3, "10 should be greater than uint.MaxValue - 2");
            Assert.True(0 > sn2, "0 should be greater than 2 ^ 31 + 1");

            Assert.True(sn1 + 5 == 15, "10 plus 5 should be 15");
            Assert.True(sn3 + 3 == 0, "sn3 plus 3 should be 0");
            Assert.True(sn3 + 103 == 100, "sn3 plus 103 should be 100");
            Assert.True(sn1 + 212 == sn4, "10 plus 212 = 212");
            Assert.True(sn1 - sn4 == -212, "10 - 222 = -212");
            Assert.True(sn4 - sn1 == 212, "222 - 10 = 212");
            Assert.True(sn1 - sn3 == 13, "sn1 - sn3 == 13");
            Assert.True(sn3 - sn1 == -13, "sn3 - sn1 == -13");
            Assert.True(sn1 + (-2) == 8u, "sn1 + (-2) == 8u");

            Assert.True(sn0 < sn5);
            Assert.Equal(sn5 + 1, sn6);
            Assert.Equal(sn6 - sn5, 1);
            Assert.Equal(new SequenceNumber(1u) + int.MaxValue, sn6);
            Assert.Equal(sn7 - sn8, -7);
            Assert.Equal(sn8 + 1, sn0);
            Assert.Equal(sn8 + 11, sn1);
            Assert.True(sn6 < sn8);
            Assert.True(sn0 > new SequenceNumber(2147483649u));

            // invalid comparisons
            Action<SequenceNumber, SequenceNumber> invalidCompare = (a, b) =>
                {
                    try
                    {
                        a.CompareTo(b);
                        bool temp = a < b;
                        temp = a > b;
                        temp = a <= b;
                        temp = a >= b;
                        Assert.True(false, "Invalid comparisons should fail");
                    }
                    catch (InvalidOperationException)
                    {
                    }
                };
            invalidCompare(0u, sn6);
            invalidCompare(sn6 + 2, 2u);
            invalidCompare(2147483647u, 4294967295u);
        }

        [Fact]
        public void BufferListStreamTest()
        {
            byte[] buffer = new byte[256];
            for (int i = 0; i < buffer.Length; i++)
            {
                buffer[i] = (byte)i;
            }

            ArraySegment<byte>[] segments = new ArraySegment<byte>[]
            {
                new ArraySegment<byte>(buffer, 0, 7),
                new ArraySegment<byte>(buffer, 7, 14),
                new ArraySegment<byte>(buffer, 21, 28),
                new ArraySegment<byte>(buffer, 49, 62),
                new ArraySegment<byte>(buffer, 111, 88),
                new ArraySegment<byte>(buffer, 199, 55),
                new ArraySegment<byte>(buffer, 254, 2),
            };

            BufferListStream stream = new BufferListStream(segments);
            Assert.True(stream.CanRead);
            Assert.True(stream.CanSeek);
            Assert.True(!stream.CanWrite);
            Assert.Equal(buffer.Length, stream.Length);

            stream.Seek(119, SeekOrigin.Begin);
            Assert.Equal(119, stream.Position);
            Assert.Equal(119, stream.ReadByte());

            stream.Seek(256, SeekOrigin.Begin);
            Assert.Equal(-1, stream.ReadByte());

            stream.Seek(-1, SeekOrigin.Current);
            Assert.Equal(255, stream.ReadByte());

            stream.Seek(-256, SeekOrigin.End);
            Assert.Equal(0, stream.ReadByte());

            try
            {
                stream.Seek(-198, SeekOrigin.Current);
                Assert.True(false, "Seek should fail with argument out of range exception");
            }
            catch (ArgumentOutOfRangeException)
            {
            }

            stream.Position = 120;

            // The position is 120 now
            stream.Seek(99, SeekOrigin.Current);
            Assert.Equal(219, stream.Position);
            Assert.Equal(219, stream.ReadByte());

            // The position is 220 now
            stream.Seek(-177, SeekOrigin.Current);
            Assert.Equal(43, stream.Position);
            Assert.Equal(43, stream.ReadByte());

            stream.Seek(0, SeekOrigin.Begin);
            for (int i = 0; i < buffer.Length; i++)
            {
               Assert.Equal(i, stream.Position);
               Assert.Equal(i, stream.ReadByte());
            }
            Assert.Equal(-1, stream.ReadByte());

            stream.Seek(25, SeekOrigin.Begin);
            byte[] tempBuffer = new byte[86];
            int count = stream.Read(tempBuffer, 0, tempBuffer.Length);
            Assert.Equal(tempBuffer.Length, count);
            Assert.Equal(111, stream.Position);
            Assert.Equal(111, stream.ReadByte());
            for (int i = 0; i < tempBuffer.Length; i++)
            {
               Assert.Equal(i + 25, tempBuffer[i]);
            }

            stream.Seek(25, SeekOrigin.Begin);
            tempBuffer = new byte[255];
            count = stream.Read(tempBuffer, 0, tempBuffer.Length);
            Assert.Equal(231, count);
            Assert.Equal(-1, stream.ReadByte());

            stream.Seek(25, SeekOrigin.Begin);
            bool more = false;
            ArraySegment<byte>[] buffers = stream.ReadBuffers(229, true, out more);
            Assert.True(more);
            Assert.Equal(4, buffers.Length);
            Assert.Equal(24, buffers[0].Count);
            Assert.Equal(62, buffers[1].Count);
            Assert.Equal(88, buffers[2].Count);
            Assert.Equal(55, buffers[3].Count);
            Assert.Equal(254, stream.Position);

            stream.Seek(25, SeekOrigin.Begin);
            more = false;
            buffers = stream.ReadBuffers(int.MaxValue, true, out more);
            Assert.False(more);
            Assert.Equal(5, buffers.Length);
            Assert.Equal(24, buffers[0].Count);
            Assert.Equal(62, buffers[1].Count);
            Assert.Equal(88, buffers[2].Count);
            Assert.Equal(55, buffers[3].Count);
            Assert.Equal(2, buffers[4].Count);
            Assert.Equal(256, stream.Position);

            stream.Seek(25, SeekOrigin.Begin);
            more = false;
            buffers = stream.ReadBuffers(231, false, out more);
            Assert.False(more);
            Assert.Equal(25, stream.Position);

            stream.Dispose();
            try
            {
                stream.Position = 100;
                Assert.True(false, "Stream is disposed!!");
            }
            catch (ObjectDisposedException)
            {
            }
        }

        [Fact]
        public void ByteBufferTest()
        {
            // wrapping a byte[]
            using (ByteBuffer buffer = new ByteBuffer(new byte[40]))
            {
                AssertBufferProperties(buffer, capacity: 40, size: 40, length: 0, offset: 0, writePos: 0);

                buffer.Append(36);
                AssertBufferProperties(buffer, capacity: 40, size: 4, length: 36, offset: 0, writePos: 36);

                buffer.Complete(29);
                AssertBufferProperties(buffer, capacity: 40, size: 4, length: 7, offset: 29, writePos: 36);

                TryWrite(buffer, 50, true, AmqpErrorCode.DecodeError);

                buffer.Reset();
                AssertBufferProperties(buffer, capacity: 40, size: 40, length: 0, offset: 0, writePos: 0);
            }

            // wrapping a byte[] with auto-grow
            using (ByteBuffer buffer = new ByteBuffer(new byte[40], true))
            {
                AssertBufferProperties(buffer, capacity: 40, size: 40, length: 0, offset: 0, writePos: 0);

                buffer.Append(36);
                AssertBufferProperties(buffer, capacity: 40, size: 4, length: 36, offset: 0, writePos: 36);

                buffer.Complete(29);
                AssertBufferProperties(buffer, capacity: 40, size: 4, length: 7, offset: 29, writePos: 36);

                TryWrite(buffer, 500, false, default(AmqpSymbol));
                AssertBufferProperties(buffer, capacity: 540, size: 4, length: 507, offset: 29, writePos: 536);
            }

            // wrapping an array segment
            using (ByteBuffer buffer = new ByteBuffer(new ArraySegment<byte>(new byte[66], 10, 44)))
            {
                AssertBufferProperties(buffer, capacity: 44, size: 0, length: 44, offset: 10, writePos: 54);

                buffer.Complete(29);
                AssertBufferProperties(buffer, capacity: 44, size: 0, length: 15, offset: 39, writePos: 54);

                TryWrite(buffer, 50, true, AmqpErrorCode.DecodeError);

                buffer.Reset();
                AssertBufferProperties(buffer, capacity: 44, size: 44, length: 0, offset: 10, writePos: 10);
            }

            // wrapping part of byte[]
            using (ByteBuffer buffer = new ByteBuffer(new byte[66], 10, 44))
            {
                AssertBufferProperties(buffer, capacity: 44, size: 0, length: 44, offset: 10, writePos: 54);

                buffer.Complete(29);
                AssertBufferProperties(buffer, capacity: 44, size: 0, length: 15, offset: 39, writePos: 54);

                TryWrite(buffer, 50, true, AmqpErrorCode.DecodeError);

                buffer.Reset();
                AssertBufferProperties(buffer, capacity: 44, size: 44, length: 0, offset: 10, writePos: 10);
            }

            // auto-grow
            using (ByteBuffer buffer = new ByteBuffer(40, true))
            {
                AssertBufferProperties(buffer, capacity: 40, size: 40, length: 0, offset: 0, writePos: 0);

                buffer.Append(36);
                AssertBufferProperties(buffer, capacity: 40, size: 4, length: 36, offset: 0, writePos: 36);

                buffer.Complete(29);
                AssertBufferProperties(buffer, capacity: 40, size: 4, length: 7, offset: 29, writePos: 36);

                TryWrite(buffer, 10, false, default(AmqpSymbol));
                AssertBufferProperties(buffer, capacity: 80, size: 34, length: 17, offset: 29, writePos: 46);

                TryWrite(buffer, 500, false, default(AmqpSymbol));
                AssertBufferProperties(buffer, capacity: 580, size: 34, length: 517, offset: 29, writePos: 546);

                buffer.Reset();
                AssertBufferProperties(buffer, capacity: 580, size: 580, length: 0, offset: 0, writePos: 0);
            }
        }

        [Fact]
        public void CanAccessStringResources()
        {
            // access some random resource from each resx file.
            Assert.NotNull(Resources.AmqpApplicationProperties);
        }

        static void AssertBufferProperties(ByteBuffer buffer, int capacity, int size, int length, int offset, int writePos)
        {
            Assert.Equal(capacity, buffer.Capacity);
            Assert.Equal(size, buffer.Size);
            Assert.Equal(length, buffer.Length);
            Assert.Equal(offset, buffer.Offset);
            Assert.Equal(writePos, buffer.WritePos);
        }

        static void TryWrite(ByteBuffer buffer, int newData, bool fail, AmqpSymbol error)
        {
            try
            {
                AmqpBitConverter.WriteBytes(buffer, new byte[newData], 0, newData);
                if (fail)
                {
                    Assert.True(false, "write should fail because buffer is smaller");
                }
            }
            catch (AmqpException exp)
            {
                if (fail)
                {
                   Assert.Equal(AmqpErrorCode.DecodeError, exp.Error.Condition);
                }
                else
                {
                    throw;
                }
            }
        }

        sealed class Worker<T> : IWorkDelegate<T>
        {
            readonly Func<T, bool> func;

            public Worker(Func<T, bool> func)
            {
                this.func = func;
            }

            bool IWorkDelegate<T>.Invoke(T work)
            {
                return this.func(work);
            }
        }
    }
}
