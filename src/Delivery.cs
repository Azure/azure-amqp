// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Defines an AMQP delivery.
    /// </summary>
    public abstract class Delivery : IDisposable
    {
        /// <summary>
        /// Gets or sets the delivery-tag.
        /// </summary>
        public ArraySegment<byte> DeliveryTag
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the delivery-id.
        /// </summary>
        public SequenceNumber DeliveryId
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the transaction-id.
        /// </summary>
        public ArraySegment<byte> TxnId
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the settled field.
        /// </summary>
        public bool Settled
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the batchable field.
        /// </summary>
        public bool Batchable
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the delivery state.
        /// </summary>
        public DeliveryState State
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the status of delivery state changes.
        /// </summary>
        internal bool StateChanged
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the link where the delivery is sent or received.
        /// </summary>
        public AmqpLink Link
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the transfered bytes of the delivery.
        /// </summary>
        public long BytesTransfered
        {
            get;
            protected set;
        }

        /// <summary>
        /// Gets or sets the number of transfers of the delivery.
        /// </summary>
        public int Segments
        {
            get;
            protected set;
        }

        /// <summary>
        /// Gets or sets the previous delivery for creating a linked list of deliveries.
        /// </summary>
        internal Delivery Previous
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the next delivery for creating a linked list of deliveries.
        /// </summary>
        internal Delivery Next
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the message format.
        /// </summary>
        public uint? MessageFormat
        {
            get;
            set;
        }

        internal static void Add(ref Delivery first, ref Delivery last, Delivery delivery)
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

        internal static void Remove(ref Delivery first, ref Delivery last, Delivery delivery)
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

        /// <summary>
        /// Gets the payload for a given size from the current position.
        /// </summary>
        /// <param name="payloadSize">The requested size in bytes.</param>
        /// <param name="more">true if more bytes are available.</param>
        /// <returns>A buffer containing the payload.</returns>
        public abstract ByteBuffer GetPayload(int payloadSize, out bool more);

        /// <summary>
        /// Adds a buffer to the payload.
        /// </summary>
        /// <param name="payload">The buffer.</param>
        /// <param name="isLast">true if the buffer is the last segment of the payload.</param>
        public abstract void AddPayload(ByteBuffer payload, bool isLast);

        /// <summary>
        /// Advances the position in the payload.
        /// </summary>
        /// <param name="payloadSize"></param>
        public virtual void CompletePayload(int payloadSize)
        {
            this.Segments++;
            this.BytesTransfered += payloadSize;
        }

        /// <summary>
        /// Disposes the delivery.
        /// </summary>
        public abstract void Dispose();

        internal static ByteBuffer GetPayload(ByteBuffer source, int payloadSize, out bool more)
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

        internal static ByteBuffer AddPayload(ByteBuffer dest, ByteBuffer payload, bool isLast)
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
