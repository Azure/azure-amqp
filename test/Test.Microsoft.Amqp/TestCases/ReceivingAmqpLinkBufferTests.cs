// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Reflection;
    using System.Threading.Tasks;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ReceivingAmqpLinkBufferTests
    {
        static readonly FieldInfo pendingField = typeof(ReceivingAmqpLink).GetField("currentDelivery", BindingFlags.Instance | BindingFlags.NonPublic);
        static readonly FieldInfo referencesField = typeof(ByteBuffer).GetField("references", BindingFlags.Instance | BindingFlags.NonPublic);

        [TestMethod]
        public void SingleTransferPreservesMessageIdentityAndBuffer()
        {
            var link = CreateLink();
            AmqpMessage received = null;
            link.RegisterMessageListener(message => received = message);
            var transfer = new Transfer { More = false };
            Assert.IsTrue(link.CreateDelivery(transfer, out Delivery delivery));
            delivery.DeliveryTag = new ArraySegment<byte>(new byte[] { 42 });
            ByteBuffer buffer;
            using (Frame frame = CreateFrame(transfer, new byte[] { 1, 2, 3 }))
            {
                buffer = frame.RawByteBuffer;
                link.ProcessTransfer(transfer, frame, delivery, false);
                Assert.AreSame(delivery, received);
                Assert.AreSame(buffer, received.Buffer);
                Assert.IsNull(pendingField.GetValue(link));
            }

            Assert.AreEqual(1, (int)referencesField.GetValue(buffer));
            Assert.AreEqual(3L, received.BytesTransfered);
            Assert.AreEqual((byte)42, received.DeliveryTag.Array[0]);
            link.Abort();
            Assert.IsNotNull(buffer.Buffer);
            received.Dispose();
            Assert.IsNull(buffer.Buffer);
        }

        [TestMethod]
        public void MultipleTransfersDeferAssemblyAndReleaseSourceBuffers()
        {
            var link = CreateLink();
            AmqpMessage received = null;
            link.RegisterMessageListener(message => received = message);
            var first = new Transfer { More = true };
            Assert.IsTrue(link.CreateDelivery(first, out Delivery delivery));
            ByteBuffer firstBuffer;
            using (Frame frame = CreateFrame(first, new byte[] { 1, 2 }))
            {
                firstBuffer = frame.RawByteBuffer;
                link.ProcessTransfer(first, frame, delivery, false);
            }

            Assert.IsNull(((AmqpMessage)delivery).Buffer);
            Assert.IsNotNull(firstBuffer.Buffer);
            Assert.AreEqual(1, (int)referencesField.GetValue(firstBuffer));

            ByteBuffer middleBuffer;
            using (Frame frame = CreateFrame(first, new byte[] { 3, 4, 5 }))
            {
                middleBuffer = frame.RawByteBuffer;
                Assert.IsFalse(link.CreateDelivery(first, out Delivery continued));
                Assert.AreSame(delivery, continued);
                link.ProcessTransfer(first, frame, continued, false);
            }

            var last = new Transfer { More = false };
            ByteBuffer lastBuffer;
            using (Frame frame = CreateFrame(last, new byte[] { 6 }))
            {
                lastBuffer = frame.RawByteBuffer;
                Assert.IsFalse(link.CreateDelivery(last, out Delivery continued));
                link.ProcessTransfer(last, frame, continued, false);
            }

            Assert.AreSame(delivery, received);
            Assert.AreEqual(6L, received.BytesTransfered);
            Assert.AreEqual(6, received.Buffer.Capacity);
            Assert.AreEqual(1, (int)referencesField.GetValue(received.Buffer));
            var actual = new byte[received.Buffer.Length];
            Array.Copy(received.Buffer.Buffer, received.Buffer.Offset, actual, 0, actual.Length);
            CollectionAssert.AreEqual(new byte[] { 1, 2, 3, 4, 5, 6 }, actual);
            Assert.IsNull(firstBuffer.Buffer);
            Assert.IsNull(middleBuffer.Buffer);
            Assert.IsNull(lastBuffer.Buffer);
            received.Dispose();
            link.Abort();
        }

        [TestMethod]
        public void AbortDefersCleanupUntilActiveTransferReleases()
        {
            var link = CreateLink();
            var transfer = new Transfer { More = true };
            link.CreateDelivery(transfer, out Delivery delivery);
            ByteBuffer firstBuffer;
            using (Frame frame = CreateFrame(transfer, new byte[] { 1, 2 }))
            {
                firstBuffer = frame.RawByteBuffer;
                link.ProcessTransfer(transfer, frame, delivery, false);
            }

            object pending = pendingField.GetValue(link);
            Assert.IsTrue((bool)Invoke(pending, "TryAddReference"));
            link.Abort();
            Assert.IsNull(pendingField.GetValue(link));
            Assert.IsNotNull(firstBuffer.Buffer);

            // The active receive operation can still register its frame after abort detaches it.
            var nextBuffer = new ByteBuffer(2, false);
            nextBuffer.Append(2);
            Invoke(pending, "AddPayload", nextBuffer);
            nextBuffer.Dispose();
            Assert.IsNotNull(nextBuffer.Buffer);
            Invoke(pending, "Release");
            Assert.IsNull(firstBuffer.Buffer);
            Assert.IsNull(nextBuffer.Buffer);
            Assert.IsFalse((bool)Invoke(pending, "TryAddReference"));
        }

        [TestMethod]
        public void AbortBeforeTransferPreventsDelivery()
        {
            var link = CreateLink();
            link.RegisterMessageListener(message => Assert.Fail("Aborted delivery reached the listener."));
            var transfer = new Transfer { More = false };
            link.CreateDelivery(transfer, out Delivery delivery);
            object pending = pendingField.GetValue(link);
            link.Abort();
            Assert.IsFalse((bool)Invoke(pending, "TryAddReference"));
            ByteBuffer buffer;
            using (Frame frame = CreateFrame(transfer, new byte[] { 1 }))
            {
                buffer = frame.RawByteBuffer;
                link.ProcessTransfer(transfer, frame, delivery, false);
            }

            Assert.IsNull(buffer.Buffer);
        }

        [TestMethod]
        public void AbortAfterHandoffDoesNotReleaseConsumerMessage()
        {
            var link = CreateLink();
            AmqpMessage received = null;
            link.RegisterMessageListener(message =>
            {
                received = message;
                link.Abort();
                Assert.IsNotNull(message.Buffer.Buffer);
            });
            var transfer = new Transfer { More = false };
            link.CreateDelivery(transfer, out Delivery delivery);
            using (Frame frame = CreateFrame(transfer, new byte[] { 1, 2 }))
            {
                link.ProcessTransfer(transfer, frame, delivery, false);
            }

            Assert.AreEqual(1, (int)referencesField.GetValue(received.Buffer));
            received.Dispose();
            Assert.IsNull(received.Buffer.Buffer);
        }

        [TestMethod]
        public void MessageSizeLimitIncludesAllPendingTransfers()
        {
            var link = CreateLink();
            link.Settings.MaxMessageSize = 3;
            var transfer = new Transfer { More = true };
            link.CreateDelivery(transfer, out Delivery delivery);
            ByteBuffer firstBuffer;
            using (Frame frame = CreateFrame(transfer, new byte[] { 1, 2 }))
            {
                firstBuffer = frame.RawByteBuffer;
                link.ProcessTransfer(transfer, frame, delivery, false);
            }

            ByteBuffer lastBuffer;
            using (Frame frame = CreateFrame(new Transfer { More = false }, new byte[] { 3, 4 }))
            {
                lastBuffer = frame.RawByteBuffer;
                Assert.ThrowsException<AmqpException>(() => link.ProcessTransfer((Transfer)frame.Command, frame, delivery, false));
            }

            Assert.IsNull(lastBuffer.Buffer);
            link.Abort();
            Assert.IsNull(firstBuffer.Buffer);
        }

        [TestMethod]
        public async Task ConcurrentCreationAndAbortReturnCapturedDelivery()
        {
            for (int i = 0; i < 100; i++)
            {
                var link = CreateLink();
                var transfer = new Transfer();
                Task<Delivery> create = Task.Run(() =>
                {
                    link.CreateDelivery(transfer, out Delivery delivery);
                    return delivery;
                });
                await Task.WhenAll(create, Task.Run(() => link.Abort()));
                Assert.IsNotNull(create.Result);

                // Late publication is allowed; release its ownership explicitly in the test.
                object pending = pendingField.GetValue(link);
                if (pending != null)
                {
                    Assert.AreSame(create.Result, pending.GetType().GetProperty("Message").GetValue(pending));
                    Invoke(pending, "Release");
                }
            }
        }

        static ReceivingAmqpLink CreateLink()
        {
            return new ReceivingAmqpLink(new AmqpLinkSettings { Role = true, LinkName = "buffer-lifetime" });
        }

        static Frame CreateFrame(Transfer transfer, byte[] payload)
        {
            transfer.Handle = 0;
            ByteBuffer buffer = Frame.EncodeCommand(FrameType.Amqp, 0, transfer, payload.Length);
            AmqpBitConverter.WriteBytes(buffer, payload, 0, payload.Length);
            var frame = new Frame();
            frame.Decode(buffer);
            return frame;
        }

        static object Invoke(object pending, string method, params object[] args)
        {
            return pending.GetType().GetMethod(method).Invoke(pending, args);
        }
    }
}
