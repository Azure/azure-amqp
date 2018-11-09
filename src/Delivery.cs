// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp.Framing;

    public abstract class Delivery : IDisposable
    {
        volatile bool settled;
        volatile bool stateChanged;

        public List<ByteBuffer> RawByteBuffers
        {
            get;
            protected set;
        }

        public ArraySegment<byte> DeliveryTag
        {
            get;
            set;
        }

        public SequenceNumber DeliveryId
        {
            get;
            set;
        }

        public ArraySegment<byte> TxnId
        {
            get;
            set;
        }

        public bool Settled
        {
            get { return this.settled; }
            set { this.settled = value; }
        }

        public bool Batchable
        {
            get;
            set;
        }

        public DeliveryState State
        {
            get;
            set;
        }

        public bool StateChanged 
        {
            get { return this.stateChanged; }
            set { this.stateChanged = value; }
        }

        public AmqpLink Link
        {
            get;
            set;
        }

        public long BytesTransfered
        {
            get;
            protected set;
        }

        public Delivery Previous
        {
            get;
            set;
        }

        public Delivery Next
        {
            get;
            set;
        }

        public uint? MessageFormat
        {
            get; 
            set; 
        }

        public static void Add(ref Delivery first, ref Delivery last, Delivery delivery)
        {
            Fx.Assert(delivery.Previous == null && delivery.Next == null, "delivery is already in a list");
            if (first == null)
            {
                Fx.Assert(last == null, "last must be null when first is null");
                first = last = delivery;
            }
            else
            {
                last.Next = delivery;
                delivery.Previous = last;
                last = delivery;
            }
        }

        public static void Remove(ref Delivery first, ref Delivery last, Delivery delivery)
        {
            if (delivery == first)
            {
                first = delivery.Next;
                if (first == null)
                {
                    last = null;
                }
                else
                {
                    first.Previous = null;
                }
            }
            else if (delivery == last)
            {
                last = delivery.Previous;
                last.Next = null;
            }
            else if (delivery.Previous != null && delivery.Next != null)
            {
                delivery.Previous.Next = delivery.Next;
                delivery.Next.Previous = delivery.Previous;
            }

            delivery.Previous = null;
            delivery.Next = null;
        }

        public void CompletePayload(int payloadSize)
        {
            this.BytesTransfered += payloadSize;
            this.OnCompletePayload(payloadSize);
        }

        public virtual void AddPayload(ByteBuffer payload, bool isLast)
        {
            throw new InvalidOperationException();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Releases unmanaged resources and optionally releases managed resources.
        /// </summary>
        /// <param name="disposing">
        /// true to release both managed and unmanaged resources;
        /// false to release only unmanaged resources.
        /// </param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing && this.RawByteBuffers != null)
            {
                foreach (ByteBuffer rawByteBuffer in this.RawByteBuffers)
                {
                    rawByteBuffer.Dispose();
                }
            }
        }

        public void PrepareForSend()
        {
            this.BytesTransfered = 0;
        }

        public abstract ArraySegment<byte>[] GetPayload(int payloadSize, out bool more);

        protected abstract void OnCompletePayload(int payloadSize);
    }
}
