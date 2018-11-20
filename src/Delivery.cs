// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    public abstract class Delivery : IDisposable
    {
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
            get;
            set;
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
            get;
            set;
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

        public int Segments
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

        public abstract ByteBuffer GetPayload(int payloadSize, out bool more);

        public abstract void AddPayload(ByteBuffer payload, bool isLast);

        public virtual void CompletePayload(int payloadSize)
        {
            this.Segments++;
            this.BytesTransfered += payloadSize;
        }

        public abstract void Dispose();

        protected static ByteBuffer GetPayload(ByteBuffer source, int payloadSize, out bool more)
        {
            int size;
            if (source.Length <= payloadSize)
            {
                size = source.Length;
                more = false;
            }
            else
            {
                size = payloadSize;
                more = true;
            }

            return source.GetSlice(source.Offset, size);
        }

        protected static ByteBuffer AddPayload(ByteBuffer dest, ByteBuffer payload, bool isLast)
        {
            if (dest == null && isLast)
            {
                // this should be the most common case: 1 transfer
                dest = payload.AddReference();
            }
            else
            {
                // multi-transfer message: merge into one buffer
                // the individual transfer buffers are disposed
                if (dest == null)
                {
                    dest = new ByteBuffer(payload.Length * 2, true);
                }

                AmqpBitConverter.WriteBytes(dest, payload.Buffer, payload.Offset, payload.Length);
            }

            return dest;
        }
    }
}
