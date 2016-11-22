// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    public class RequestResponseAmqpLink : AmqpObject
    {
        static readonly TimeSpan OperationTimeout = AmqpConstants.DefaultTimeout;
        static readonly AsyncCallback onSenderOpen = OnSenderOpen;
        static readonly AsyncCallback onReceiverOpen = OnReceiverOpen;
        static readonly AsyncCallback onSenderClose = OnSenderClose;
        static readonly AsyncCallback onReceiverClose = OnReceiverClose;
        readonly Address replyTo;
        readonly SendingAmqpLink sender;
        readonly ReceivingAmqpLink receiver;
        readonly WorkCollection<MessageId, RequestAsyncResult, AmqpMessage> inflightRequests;
        Dictionary<string, object> requestProperties;
        long nextRequestId;

        public RequestResponseAmqpLink(string type, AmqpSession session, string address, Fields properties)
            : this(type, null, session, address, properties)
        {
        }

        public RequestResponseAmqpLink(string type, string name, AmqpSession session, string address)
            : this(type, name, session, address, null)
        {
        }

        public RequestResponseAmqpLink(string type, string name, AmqpSession session, string address, Fields properties)
            : base(type)
        {
            this.Name = name ?? string.Format(CultureInfo.InvariantCulture, "{0}:{1}:{2}", session.Connection.Identifier, session.Identifier, this.Identifier);
            this.replyTo = Guid.NewGuid().ToString("N");

            AmqpLinkSettings senderSettings = new AmqpLinkSettings();
            senderSettings.Role = false;
            senderSettings.LinkName = this.Name + ":sender";
            senderSettings.SettleType = SettleMode.SettleOnSend;
            senderSettings.Source = new Source() { Address = Guid.NewGuid().ToString("N") };
            senderSettings.Target = new Target() { Address = address };
            senderSettings.Properties = properties;
            this.sender = new SendingAmqpLink(session, senderSettings);
            this.sender.Closed += new EventHandler(OnLinkClosed);

            AmqpLinkSettings receiverSettings = new AmqpLinkSettings();
            receiverSettings.Role = true;
            receiverSettings.LinkName = this.Name + ":receiver";
            receiverSettings.SettleType = SettleMode.SettleOnSend;
            receiverSettings.Source = new Source() { Address = address };
            receiverSettings.TotalLinkCredit = 50;
            receiverSettings.AutoSendFlow = true;
            receiverSettings.Target = new Target() { Address = this.replyTo };
            if (properties != null)
            {
                receiverSettings.Properties = new Fields();
                receiverSettings.Properties.Merge(properties);
            }

            this.receiver = new ReceivingAmqpLink(session, receiverSettings);
            this.receiver.RegisterMessageListener(this.OnResponseMessage);
            this.receiver.Closed += new EventHandler(OnLinkClosed);

            this.inflightRequests = new WorkCollection<MessageId, RequestAsyncResult, AmqpMessage>();
        }

        public string Name { get; private set; }

        public SendingAmqpLink SendingLink
        {
            get
            {
                return this.sender;
            }
        }

        public ReceivingAmqpLink ReceivingLink
        {
            get
            {
                return this.receiver;
            }
        }

        public Dictionary<string, object> RequestProperties
        {
            get
            {
                if (this.requestProperties == null)
                {
                    lock (ThisLock)
                    {
                        if (this.requestProperties == null)
                        {
                            this.requestProperties = new Dictionary<string, object>();
                        }

                    }
                }

                return this.requestProperties;
            }
        }

        public AmqpSession Session
        {
            get
            {
                return this.sender.Session;
            }
        }

        public Task<AmqpMessage> RequestAsync(AmqpMessage request, TimeSpan timeout)
        {
            return TaskHelpers.CreateTask(
                (c, s) => ((RequestResponseAmqpLink)s).BeginRequest(request, timeout, c, s),
                (r) => ((RequestResponseAmqpLink)r.AsyncState).EndRequest(r),
                this);
        }

        public IAsyncResult BeginRequest(AmqpMessage request, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return new RequestAsyncResult(this, request, timeout, callback, state);
        }

        public AmqpMessage EndRequest(IAsyncResult result)
        {
            return RequestAsyncResult.End(result);
        }

        public void SendProperties(Fields fields)
        {
            this.receiver.SendProperties(fields);
            this.sender.SendProperties(fields);
        }

        protected override bool OpenInternal()
        {
            IAsyncResult senderResult = this.sender.BeginOpen(OperationTimeout, onSenderOpen, this);
            IAsyncResult receiverResult = this.receiver.BeginOpen(OperationTimeout, onReceiverOpen, this);
            return senderResult.CompletedSynchronously && receiverResult.CompletedSynchronously;
        }

        protected override bool CloseInternal()
        {
            this.inflightRequests.Abort();
            IAsyncResult senderResult = this.sender.BeginClose(OperationTimeout, onSenderClose, this);
            IAsyncResult receiverResult = this.receiver.BeginClose(OperationTimeout, onReceiverClose, this);
            return senderResult.CompletedSynchronously && receiverResult.CompletedSynchronously;
        }

        protected override void AbortInternal()
        {
            this.sender.Abort();
            this.receiver.Abort();
            this.inflightRequests.Abort();
        }

        static void OnSenderOpen(IAsyncResult result)
        {
            RequestResponseAmqpLink thisPtr = (RequestResponseAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.sender, result, true);
        }

        static void OnReceiverOpen(IAsyncResult result)
        {
            RequestResponseAmqpLink thisPtr = (RequestResponseAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.receiver, result, true);
        }

        static void OnSenderClose(IAsyncResult result)
        {
            RequestResponseAmqpLink thisPtr = (RequestResponseAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.sender, result, false);
        }

        static void OnReceiverClose(IAsyncResult result)
        {
            RequestResponseAmqpLink thisPtr = (RequestResponseAmqpLink)result.AsyncState;
            thisPtr.OnOperationComplete(thisPtr.receiver, result, false);
        }

        void OnLinkClosed(object sender, EventArgs e)
        {
            this.SafeClose();
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

        void OnResponseMessage(AmqpMessage response)
        {
            this.receiver.DisposeDelivery(response, true, AmqpConstants.AcceptedOutcome);
            if (response.Properties != null &&
                response.Properties.CorrelationId != null)
            {
                this.inflightRequests.CompleteWork(response.Properties.CorrelationId, false, response);
            }
        }

        sealed class RequestAsyncResult : TimeoutAsyncResult<MessageId>, IWork<AmqpMessage>
        {
            readonly RequestResponseAmqpLink parent;
            readonly MessageId requestId;
            AmqpMessage request;
            AmqpMessage response;

            public RequestAsyncResult(RequestResponseAmqpLink parent, AmqpMessage request, TimeSpan timeout, AsyncCallback callback, object state)
                : base(timeout, callback, state)
            {
                this.parent = parent;
                this.request = request;

                this.requestId = "r" + (ulong)Interlocked.Increment(ref this.parent.nextRequestId);
                this.request.Properties.MessageId = this.requestId;
                this.request.Properties.ReplyTo = this.parent.replyTo;
                this.parent.inflightRequests.StartWork(this.requestId, this);
            }

            protected override MessageId Target
            {
                get { return this.requestId; }
            }

            public static AmqpMessage End(IAsyncResult result)
            {
                return AsyncResult.End<RequestAsyncResult>(result).response;
            }

            public void Start()
            {
                this.SetTimer();
                this.parent.sender.SendMessageNoWait(this.request, AmqpConstants.EmptyBinary, AmqpConstants.NullBinary);
                this.request = null;
            }

            public void Done(bool completedSynchronously, AmqpMessage response)
            {
                this.response = response;
                this.CompleteSelf(completedSynchronously);
            }

            public void Cancel(bool completedSynchronously, Exception exception)
            {
                this.CompleteSelf(completedSynchronously, exception);
            }

            protected override void CompleteOnTimer()
            {
                RequestAsyncResult thisPtr;
                if (this.parent.inflightRequests.TryRemoveWork(this.requestId, out thisPtr))
                {
                    base.CompleteOnTimer();
                }
            }
        }
    }
}