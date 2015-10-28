// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using System;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Tracing;

    sealed class Controller
    {
        SendingAmqpLink sendLink;
        long messageTag;

        public static AmqpMessage CreateCommandMessage(IAmqpSerializable command)
        {
            AmqpValue value = new AmqpValue() { Value = command };
            return AmqpMessage.Create(value);
        }

        public IAsyncResult BeginOpen(AmqpSession session, TimeSpan timeout, AsyncCallback callback, object state)
        {
            string uniqueueName = Guid.NewGuid().ToString("N");
            Source source = new Source();
            source.Address = uniqueueName;
            source.DistributionMode = DistributionMode.Move;

            Coordinator coordinator = new Coordinator();
            AmqpLinkSettings settings = new AmqpLinkSettings();
            settings.Source = source;
            settings.Target = coordinator;
            settings.LinkName = uniqueueName;
            settings.Role = false;

            this.sendLink = new SendingAmqpLink(session, settings);
            return this.sendLink.BeginOpen(timeout, callback, state);
        }

        public void EndOpen(IAsyncResult result)
        {
            this.sendLink.EndOpen(result);
        }

        public void Open(AmqpSession session, TimeSpan timeout)
        {
            this.EndOpen(this.BeginOpen(session, timeout, null, null));
        }

        public IAsyncResult BeginClose(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.sendLink.BeginClose(timeout, callback, state);
        }

        public void EndClose(IAsyncResult result)
        {
            this.sendLink.EndClose(result);
            this.sendLink = null;
        }

        public void Close(TimeSpan timeout)
        {
            this.EndClose(this.BeginClose(timeout, null, null));
        }

        public IAsyncResult BeginDeclare(TimeSpan timeout, AsyncCallback callback, object state)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "BeginDeclare");
            Declare declare = new Declare();
            AmqpMessage message = Controller.CreateCommandMessage(declare);
            return this.sendLink.BeginSendMessage(message, this.GetDeliveryTag(), AmqpConstants.NullBinary, timeout, callback, state);
        }

        public ArraySegment<byte> EndDeclare(IAsyncResult result)
        {
            DeliveryState deliveryState = this.sendLink.EndSendMessage(result);
            this.ThrowIfRejected(deliveryState);
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "EndDeclare");
            return ((Declared)deliveryState).TxnId;
        }

        public IAsyncResult BeginDischange(ArraySegment<byte> txnId, bool fail, TimeSpan timeout, AsyncCallback callback, object state)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "BeginDischange");
            Discharge discharge = new Discharge()
            {
                TxnId = txnId,
                Fail = fail
            };

            AmqpMessage message = Controller.CreateCommandMessage(discharge);
            return this.sendLink.BeginSendMessage(message, this.GetDeliveryTag(), AmqpConstants.NullBinary, timeout, callback, state);
        }

        public void EndDischarge(IAsyncResult result)
        {
            DeliveryState deliveryState = this.sendLink.EndSendMessage(result);
            this.ThrowIfRejected(deliveryState);
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "EndDischange");
        }

        public override string ToString()
        {
            return "controller";
        }

        void ThrowIfRejected(DeliveryState deliveryState)
        {
            if (deliveryState.DescriptorCode == Rejected.Code)
            {
                Rejected rejected = (Rejected)deliveryState;
                throw AmqpException.FromError(rejected.Error);
            }
        }

        ArraySegment<byte> GetDeliveryTag()
        {
            long tag = Interlocked.Increment(ref this.messageTag);
            return new ArraySegment<byte>(BitConverter.GetBytes(tag));
        }
    }
}
