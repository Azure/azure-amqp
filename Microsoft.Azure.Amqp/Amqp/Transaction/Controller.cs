// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Transaction
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    public sealed class Controller : AmqpObject
    {
        readonly SendingAmqpLink controllerLink;
        long messageTag;

        readonly TimeSpan operationTimeout;

        public Controller(AmqpSession amqpSession, TimeSpan operationTimeout)
            : base("controller")
        {
            this.operationTimeout = operationTimeout;
            string uniqueueName = Guid.NewGuid().ToString("N");
            var source = new Source
            {
                Address = uniqueueName,
                DistributionMode = DistributionMode.Move
            };
            var coordinator = new Coordinator();
            var settings = new AmqpLinkSettings
            {
                Source = source,
                Target = coordinator,
                LinkName = uniqueueName,
                Role = false
            };

            this.controllerLink = new SendingAmqpLink(amqpSession, settings);
        }

        public Task<ArraySegment<byte>> DeclareAsync()
        {
            return this.DeclareAsync(CancellationToken.None);
        }

        public async Task<ArraySegment<byte>> DeclareAsync(CancellationToken cancellationToken)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "BeginDeclare");
            Declare declare = new Declare();

            AmqpMessage message = Controller.CreateCommandMessage(declare);
            DeliveryState deliveryState = await Task<DeliveryState>.Factory.FromAsync(
                (m, k, c, s) =>
                {
                    var thisPtr = (Controller)s;
                    return thisPtr.controllerLink.BeginSendMessage(m, thisPtr.GetDeliveryTag(), AmqpConstants.NullBinary, thisPtr.operationTimeout, k, c, s);
                },
                r => ((Controller)r.AsyncState).controllerLink.EndSendMessage(r),
                message,
                cancellationToken,
                this);

            this.ThrowIfRejected(deliveryState);
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "EndDeclare");
            return ((Declared)deliveryState).TxnId;
        }

        public Task DischargeAsync(ArraySegment<byte> txnId, bool fail)
        {
            return this.DischargeAsync(txnId, fail, CancellationToken.None);
        }

        public async Task DischargeAsync(ArraySegment<byte> txnId, bool fail, CancellationToken cancellationToken)
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "BeginDischange");
            Discharge discharge = new Discharge
            {
                TxnId = txnId,
                Fail = fail
            };

            AmqpMessage message = Controller.CreateCommandMessage(discharge);
            DeliveryState deliveryState = await Task<DeliveryState>.Factory.FromAsync(
                (m, k, c, s) =>
                {
                    var thisPtr = (Controller)s;
                    return thisPtr.controllerLink.BeginSendMessage(m, thisPtr.GetDeliveryTag(), AmqpConstants.NullBinary, thisPtr.operationTimeout, k, c, s);
                },
                r => ((Controller)r.AsyncState).controllerLink.EndSendMessage(r),
                message,
                cancellationToken,
                this);

            this.ThrowIfRejected(deliveryState);
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Execute, "EndDischange");
        }

        public override string ToString()
        {
            return "controller";
        }

        protected override bool OpenInternal()
        {
            var result = this.controllerLink.BeginOpen(this.operationTimeout, OnLinkOpen, this);
            return result.IsCompleted;
        }

        protected override bool CloseInternal()
        {
            this.controllerLink.SafeClose();
            this.controllerLink.Session.SafeClose();
            return true;
        }

        protected override void AbortInternal()
        {
            this.controllerLink.Abort();
            this.controllerLink.Session.Abort();
        }

        static AmqpMessage CreateCommandMessage(IAmqpSerializable command)
        {
            AmqpValue value = new AmqpValue { Value = command };
            return AmqpMessage.Create(value);
        }

        static void OnLinkOpen(IAsyncResult asyncResult)
        {
            var thisPtr = (Controller) asyncResult.AsyncState;
            Exception ex = null;
            try
            {
                thisPtr.controllerLink.EndOpen(asyncResult);
                thisPtr.controllerLink.SafeAddClosed((sender, args) => { thisPtr.SafeClose(); });
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                ex = exception;
            }

            if (!asyncResult.CompletedSynchronously)
            {
                thisPtr.CompleteOpen(false, ex);
            }
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
