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

    /// <summary>
    /// A duplex communication channel over a pair of AMQP links in different
    /// direction.
    /// </summary>
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

        /// <summary>
        /// Initializes the request response channel.
        /// </summary>
        /// <param name="type">A prefix of the object name for debugging purposes.</param>
        /// <param name="session">The session in which the links are created.</param>
        /// <param name="address">The node address.</param>
        /// <param name="properties">The properties to send in attach performatives.</param>
        public RequestResponseAmqpLink(string type, AmqpSession session, string address, Fields properties)
            : this(type, null, session, address, properties)
        {
        }

        /// <summary>
        /// Initializes the request response channel.
        /// </summary>
        /// <param name="type">A prefix of the object name for debugging purposes.</param>
        /// <param name="name">The object name.</param>
        /// <param name="session">The session in which the links are created.</param>
        /// <param name="address">The node address.</param>
        public RequestResponseAmqpLink(string type, string name, AmqpSession session, string address)
            : this(type, name, session, address, null)
        {
        }

        /// <summary>
        /// Initializes the request response channel.
        /// </summary>
        /// <param name="type">A prefix of the object name for debugging purposes.</param>
        /// <param name="name">The object name.</param>
        /// <param name="session">The session in which the links are created.</param>
        /// <param name="address">The node address.</param>
        /// <param name="properties">The properties to send in attach performatives.</param>
        public RequestResponseAmqpLink(string type, string name, AmqpSession session, string address, Fields properties)
            : base(type)
        {
            this.Name = name ?? string.Format(CultureInfo.InvariantCulture, "duplex{0}:{1}:{2}", session.Connection.Identifier, session.Identifier, this.Identifier);
            this.replyTo = Guid.NewGuid().ToString("N");

            try
            {
                AmqpLinkSettings senderSettings = new AmqpLinkSettings();
                senderSettings.Role = false;
                senderSettings.LinkName = this.Name + ":sender";
                senderSettings.SettleType = SettleMode.SettleOnSend;
                senderSettings.Source = new Source();
                senderSettings.Target = new Target() { Address = address };
                senderSettings.Properties = properties;
                this.sender = new SendingAmqpLink(session, senderSettings);
                this.sender.SafeAddClosed(OnLinkClosed);

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
                this.receiver.SafeAddClosed(OnLinkClosed);
            }
            catch
            {
                this.sender?.SafeClose();
                this.receiver?.SafeClose();
                throw;
            }

            this.inflightRequests = new WorkCollection<MessageId, RequestAsyncResult, AmqpMessage>();
        }

        /// <summary>
        /// Gets the object name.
        /// </summary>
        public string Name { get; private set; }

        internal SendingAmqpLink SendingLink
        {
            get
            {
                return this.sender;
            }
        }

        internal ReceivingAmqpLink ReceivingLink
        {
            get
            {
                return this.receiver;
            }
        }

        /// <summary>
        /// The default properties to include in every request message
        /// application-properties section.
        /// </summary>
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

        /// <summary>
        /// Gets the <see cref="AmqpSession"/> object of the links.
        /// </summary>
        public AmqpSession Session
        {
            get
            {
                return this.sender.Session;
            }
        }

        /// <summary>
        /// Starts an asynchronous request.
        /// </summary>
        /// <param name="request">The request message.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <returns>A task representing the asynchronous operation.</returns>
        public Task<AmqpMessage> RequestAsync(AmqpMessage request, TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                static (r, t, c, s) => ((RequestResponseAmqpLink)s).BeginRequest(r, AmqpConstants.NullBinary, t, CancellationToken.None, c, s),
                static (r) => ((RequestResponseAmqpLink)r.AsyncState).EndRequest(r),
                request,
                timeout,
                this);
        }

        /// <summary>
        /// Starts an asynchronous request.
        /// </summary>
        /// <param name="request">The request message.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>A task representing the asynchronous operation.</returns>
        public Task<AmqpMessage> RequestAsync(AmqpMessage request, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                static (r, k, c, s) => ((RequestResponseAmqpLink)s).BeginRequest(r, AmqpConstants.NullBinary, TimeSpan.MaxValue, k, c, s),
                static (r) => ((RequestResponseAmqpLink)r.AsyncState).EndRequest(r),
                request,
                cancellationToken,
                this);
        }

        /// <summary>
        /// Begins an asynchronous request.
        /// </summary>
        /// <param name="request">The request message.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The object that is associated with the operation.</param>
        /// <returns>An <see cref="IAsyncResult"/> representing the asynchronous operation.</returns>
        public IAsyncResult BeginRequest(AmqpMessage request, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginRequest(request, AmqpConstants.NullBinary, timeout, callback, state);
        }

        /// <summary>
        /// Begins an asynchronous request.
        /// </summary>
        /// <param name="request">The request message.</param>
        /// <param name="txnId">The transaction id.</param>
        /// <param name="timeout">The operation timeout.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The object that is associated with the operation.</param>
        /// <returns>An <see cref="IAsyncResult"/> representing the asynchronous operation.</returns>
        public IAsyncResult BeginRequest(AmqpMessage request, ArraySegment<byte> txnId, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginRequest(request, txnId, timeout, CancellationToken.None, callback, state);
        }

        /// <summary>
        /// Ends an asynchronous request.
        /// </summary>
        /// <param name="result">The <see cref="IAsyncResult"/> object returned by the begin method.</param>
        /// <returns>The response message.</returns>
        public AmqpMessage EndRequest(IAsyncResult result)
        {
            return RequestAsyncResult.End(result);
        }

        /// <summary>
        /// Sends a set of properties to the remote peer in a flow performative.
        /// </summary>
        /// <param name="fields">The properties.</param>
        public void SendProperties(Fields fields)
        {
            this.receiver.SendProperties(fields);
            this.sender.SendProperties(fields);
        }

        /// <summary>
        /// Opens the object.
        /// </summary>
        /// <returns>true if open is completed, otherwise false.</returns>
        protected override bool OpenInternal()
        {
            var state = new OperationState() { Owner = this };
            IAsyncResult senderResult = this.sender.BeginOpen(OperationTimeout, onSenderOpen, state);
            IAsyncResult receiverResult = this.receiver.BeginOpen(OperationTimeout, onReceiverOpen, state);
            return senderResult.CompletedSynchronously && receiverResult.CompletedSynchronously;
        }

        /// <summary>
        /// Closes the object.
        /// </summary>
        /// <returns>true if close is completed, otherwise false.</returns>
        protected override bool CloseInternal()
        {
            this.inflightRequests.Abort();
            var state = new OperationState() { Owner = this };
            IAsyncResult senderResult = this.sender.BeginClose(OperationTimeout, onSenderClose, state);
            IAsyncResult receiverResult = this.receiver.BeginClose(OperationTimeout, onReceiverClose, state);
            return senderResult.CompletedSynchronously && receiverResult.CompletedSynchronously;
        }

        /// <summary>
        /// Aborts the object. All pending requests are canceled.
        /// </summary>
        protected override void AbortInternal()
        {
            this.sender.Abort();
            this.receiver.Abort();
            this.inflightRequests.Abort();
        }

        static void OnSenderOpen(IAsyncResult result)
        {
            OnOperationComplete(result, true, true);
        }

        static void OnReceiverOpen(IAsyncResult result)
        {
            OnOperationComplete(result, false, true);
        }

        static void OnSenderClose(IAsyncResult result)
        {
            OnOperationComplete(result, true, false);
        }

        static void OnReceiverClose(IAsyncResult result)
        {
            OnOperationComplete(result, false, false);
        }

        void OnLinkClosed(object sender, EventArgs e)
        {
            this.SafeClose();
        }

        static void OnOperationComplete(IAsyncResult result, bool isSender, bool isOpen)
        {
            OperationState state = (OperationState)result.AsyncState;
            RequestResponseAmqpLink thisPtr = state.Owner;
            AmqpLink link = isSender ? (AmqpLink)thisPtr.sender : thisPtr.receiver;

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
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                completeException = exception;
            }

            if (state.Complete() || completeException != null)
            {
                if (isOpen)
                {
                    thisPtr.CompleteOpen(false, completeException);
                }
                else
                {
                    thisPtr.CompleteClose(false, completeException);
                }
            }
        }

        IAsyncResult BeginRequest(AmqpMessage request, ArraySegment<byte> txnId, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            var requestResult = new RequestAsyncResult(this, request, txnId, timeout, cancellationToken, callback, state);
            return requestResult;
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

        sealed class OperationState
        {
            int pending = 2;

            public RequestResponseAmqpLink Owner { get; set; }

            public bool Complete()
            {
                return Interlocked.Decrement(ref this.pending) <= 0;
            }
        }

        sealed class RequestAsyncResult : TimeoutAsyncResult<MessageId>, IWork<AmqpMessage>
        {
            readonly RequestResponseAmqpLink parent;
            readonly MessageId requestId;
            readonly ArraySegment<byte> transactionId;
            AmqpMessage request;
            AmqpMessage response;

            public RequestAsyncResult(RequestResponseAmqpLink parent, AmqpMessage request, ArraySegment<byte> txnId,
                TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
                : base(timeout, cancellationToken, callback, state)
            {
                this.parent = parent;
                this.request = request;
                this.transactionId = txnId;
                this.requestId = this.parent.ToString() + (ulong)Interlocked.Increment(ref this.parent.nextRequestId);
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
                this.parent.sender.SendMessageNoWait(this.request, AmqpConstants.EmptyBinary, this.transactionId);
                this.request = null;
                this.StartTracking();
            }

            public override void Cancel()
            {
                if (this.parent.inflightRequests.TryRemoveWork(this.requestId, out _))
                {
                    this.CompleteSelf(false, new TaskCanceledException());
                }
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