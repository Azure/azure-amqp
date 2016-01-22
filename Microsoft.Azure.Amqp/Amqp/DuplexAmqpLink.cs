// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Tracing;

    sealed class DuplexAmqpLink : AmqpObject
    {
        static readonly AsyncCallback onSenderOpen = OnSenderOpen;
        static readonly AsyncCallback onReceiverOpen = OnReceiverOpen;
        static readonly AsyncCallback onSenderClose = OnSenderClose;
        static readonly AsyncCallback onReceiverClose = OnReceiverClose;
        readonly SendingAmqpLink sender;
        readonly ReceivingAmqpLink receiver;

        public DuplexAmqpLink(AmqpSession session, AmqpLinkSettings settings)
            : base("duplex")
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Create, "Create");

            var senderSettings = new AmqpLinkSettings
            {
                Role = false,
                LinkName = settings.LinkName + ":out",
                SettleType = settings.SettleType,
                Source = new Source(),
                TotalLinkCredit = settings.TotalLinkCredit,
                AutoSendFlow = settings.AutoSendFlow,
                Target = settings.Target,
                Properties = settings.Properties
            };
            this.sender = new SendingAmqpLink(session, senderSettings);

            var receiverSettings = new AmqpLinkSettings
            {
                Role = true,
                LinkName = settings.LinkName + ":in",
                SettleType = settings.SettleType,
                Source = settings.Source,
                TotalLinkCredit = settings.TotalLinkCredit,
                AutoSendFlow = settings.AutoSendFlow,
                Target = new Target(),
                Properties = settings.Properties
            };
            this.receiver = new ReceivingAmqpLink(session, receiverSettings);
            this.receiver.SetTotalLinkCredit(receiverSettings.TotalLinkCredit, true); // WHY set both here AND on settings? Follow up with Xin.

            this.sender.SafeAddClosed(this.OnLinkClosed);
            this.receiver.SafeAddClosed(this.OnLinkClosed);
        }

        public DuplexAmqpLink(SendingAmqpLink sender, ReceivingAmqpLink receiver)
            : base("duplex")
        {
            AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Create, "Create");
            this.sender = sender;
            this.receiver = receiver;

            this.sender.SafeAddClosed(this.OnLinkClosed);
            this.receiver.SafeAddClosed(this.OnLinkClosed);

            // TODO:
            ////if (this.sender.State != AmqpObjectState.Opened)
            ////{
            ////    this.SafeClose();
            ////    throw Fx.Exception.AsWarning(new ArgumentException("Sender wasn't open", "sender"));
            ////}

            ////if (this.receiver.State != AmqpObjectState.Opened)
            ////{
            ////    this.SafeClose();
            ////    throw Fx.Exception.AsWarning(new ArgumentException("Reciever wasn't open", "receiver"));
            ////}
        }

        internal SendingAmqpLink SendingLink
        {
            get { return this.sender; }
        }

        public Task<Outcome> SendMessageAsync(AmqpMessage message, ArraySegment<byte> deliveryTag, ArraySegment<byte> txnId, TimeSpan timeout)
        {
            return this.sender.SendMessageAsync(message, deliveryTag, txnId, timeout);
        }

        public void RegisterMessageListener(Action<AmqpMessage> messageListener)
        {
            this.receiver.RegisterMessageListener(messageListener);
        }

        public void DisposeMessage(AmqpMessage message, DeliveryState deliveryState, bool settled, bool batchable)
        {
            this.receiver.DisposeMessage(message, deliveryState, settled, batchable);
        }

        internal void SendProperties(Fields fields)
        {
            this.receiver.SendProperties(fields);
            this.sender.SendProperties(fields);
        }

        protected override bool OpenInternal()
        {            
            IAsyncResult senderResult = this.sender.BeginOpen(this.DefaultOpenTimeout, onSenderOpen, this);
            IAsyncResult receiverResult = this.receiver.BeginOpen(this.DefaultOpenTimeout, onReceiverOpen, this);
            return senderResult.CompletedSynchronously && receiverResult.CompletedSynchronously;
        }

        protected override bool CloseInternal()
        {
            if (this.TerminalException != null)
            {
                this.sender.SafeClose(this.TerminalException);
                this.receiver.SafeClose(this.TerminalException);
                return true;
            }

            IAsyncResult senderResult = this.sender.BeginClose(this.DefaultCloseTimeout, onSenderClose, this);
            IAsyncResult receiverResult = this.receiver.BeginClose(this.DefaultCloseTimeout, onReceiverClose, this);
            return senderResult.CompletedSynchronously && receiverResult.CompletedSynchronously;
        }

        protected override void AbortInternal()
        {
            this.sender.Abort();
            this.receiver.Abort();
        }

        static void OnSenderOpen(IAsyncResult result)
        {
            DuplexAmqpLink thisPtr = (DuplexAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.sender, result, true);
        }

        static void OnReceiverOpen(IAsyncResult result)
        {
            DuplexAmqpLink thisPtr = (DuplexAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.receiver, result, true);
        }

        static void OnSenderClose(IAsyncResult result)
        {
            DuplexAmqpLink thisPtr = (DuplexAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.sender, result, false);
        }

        static void OnReceiverClose(IAsyncResult result)
        {
            DuplexAmqpLink thisPtr = (DuplexAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.receiver, result, false);
        }

        void OnLinkClosed(object closedObject, EventArgs e)
        {
            this.SafeClose(((AmqpObject)closedObject).TerminalException);
        }

        void OnOperationComplete(AmqpObject link, IAsyncResult result, bool isOpen)
        {
            Exception completeException = null;
            try
            {
                if (isOpen)
                {
                    link.EndOpen(result);
                }
                else
                {
                    link.EndClose(result);
                }
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                completeException = exception;
            }

            bool shouldComplete = true;
            if (completeException == null)
            {
                AmqpObjectState initialState = isOpen ? AmqpObjectState.OpenSent : AmqpObjectState.CloseSent;
                lock (this.ThisLock)
                {
                    shouldComplete = this.sender.State != initialState && this.receiver.State != initialState;
                }
            }

            if (shouldComplete)
            {
                if (isOpen)
                {
                    this.CompleteOpen(false, completeException);
                }
                else
                {
                    this.CompleteClose(false, completeException);
                }
            }
        }
    }
}