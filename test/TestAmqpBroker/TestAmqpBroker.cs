﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace TestAmqpBroker
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Principal;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp.Transaction;
    using Microsoft.Azure.Amqp.Transport;

    public sealed class TestAmqpBroker : IRuntimeProvider
    {
        readonly Dictionary<string, TestQueue> queues;
        readonly Dictionary<SequenceNumber, AmqpConnection> connections;
        readonly TxnManager txnManager;
        readonly AmqpSettings settings;
        readonly TransportListener transportListener;
        readonly string containerId;
        readonly uint maxFrameSize;
        bool implicitQueue;
        int dynamicId;

        public TestAmqpBroker(IList<string> endpoints, string userInfo, string sslValue, string[] queues)
        {
            this.containerId = "TestAmqpBroker-P" + Process.GetCurrentProcess().Id;
            this.maxFrameSize = 64 * 1024;
            this.txnManager = new TxnManager();
            this.connections = new Dictionary<SequenceNumber, AmqpConnection>();
            this.queues = new Dictionary<string, TestQueue>();
            if (queues != null)
            {
                foreach (string q in queues)
                {
                    this.queues.Add(q, new TestQueue(this));
                }
            }
            else
            {
                this.implicitQueue = true;
            }

            // create and initialize AmqpSettings
            AmqpSettings settings = new AmqpSettings();
            X509Certificate2 certificate = sslValue == null ? null : GetCertificate(sslValue);
            settings.RuntimeProvider = this;

            SaslHandler saslHandler;
            if (userInfo != null)
            {
                string[] creds = userInfo.Split(':');
                string usernanme = Uri.UnescapeDataString(creds[0]);
                string password = creds.Length == 1 ? string.Empty : Uri.UnescapeDataString(creds[1]);
                saslHandler = new SaslPlainHandler(new TestPlainAuthenticator(userInfo, password));
            }
            else
            {
                saslHandler = new SaslAnonymousHandler();
            }

            SaslTransportProvider saslProvider = new SaslTransportProvider();
            saslProvider.AddHandler(saslHandler);
            saslProvider.Versions.Add(new AmqpVersion(1, 0, 0));
            settings.TransportProviders.Add(saslProvider);

            AmqpTransportProvider amqpProvider = new AmqpTransportProvider();
            amqpProvider.Versions.Add(new AmqpVersion(1, 0, 0));
            settings.TransportProviders.Add(amqpProvider);

            // create and initialize transport listeners
            TransportListener[] listeners = new TransportListener[endpoints.Count];
            for (int i = 0; i < endpoints.Count; i++)
            {
                Uri addressUri = new Uri(endpoints[i]);

                if (addressUri.Scheme.Equals(AmqpConstants.SchemeAmqps, StringComparison.OrdinalIgnoreCase))
                {
                    if (certificate == null)
                    {
                        throw new InvalidOperationException("/cert option was not set when amqps address is specified.");
                    }

                    TcpTransportSettings tcpSettings = new TcpTransportSettings() { Host = addressUri.Host, Port = addressUri.Port };
                    TlsTransportSettings tlsSettings = new TlsTransportSettings(tcpSettings) { Certificate = certificate, IsInitiator = false };
                    listeners[i] = tlsSettings.CreateListener();
                }
                else if (addressUri.Scheme.Equals(AmqpConstants.SchemeAmqp, StringComparison.OrdinalIgnoreCase))
                {
                    TcpTransportSettings tcpSettings = new TcpTransportSettings() { Host = addressUri.Host, Port = addressUri.Port };
                    listeners[i] = tcpSettings.CreateListener();
                }
#if NET45
                else if (addressUri.Scheme.Equals("ws", StringComparison.OrdinalIgnoreCase))
                {
                    WebSocketTransportSettings wsSettings = new WebSocketTransportSettings() { Uri = addressUri };
                    listeners[i] = wsSettings.CreateListener();
                }
#endif
                else
                {
                    throw new NotSupportedException(addressUri.Scheme);
                }
            }

            this.transportListener = new AmqpTransportListener(listeners, settings);
            this.settings = settings;
        }

        public void Start()
        {
            this.transportListener.Listen(this.OnAcceptTransport);
        }

        public void Stop()
        {
            this.transportListener.Close();
        }

        public void AddQueue(string queue)
        {
            this.queues.Add(queue, new TestQueue(this));
        }

        void connection_Closed(object sender, EventArgs e)
        {
            lock (this.connections)
            {
                this.connections.Remove(((AmqpConnection)sender).Identifier);
            }
        }

        void OnAcceptTransport(TransportListener listener, TransportAsyncCallbackArgs args)
        {
            AmqpConnectionSettings connectionSettings = new AmqpConnectionSettings()
            {
                ContainerId = this.containerId,
                MaxFrameSize = this.maxFrameSize
            };

            AmqpConnection connection = null;
            try
            {
                connection = this.CreateConnection(
                    args.Transport,
                    (ProtocolHeader)args.UserToken,
                    false,
                    this.settings,
                    connectionSettings);

                connection.BeginOpen(connection.DefaultOpenTimeout, this.OnConnectionOpenComplete, connection);
            }
            catch (Exception ex)
            {
                if (connection != null)
                {
                    connection.SafeClose(ex);
                }
            }
        }

        void OnConnectionOpenComplete(IAsyncResult result)
        {
            AmqpConnection connection = (AmqpConnection)result.AsyncState;
            try
            {
                connection.EndOpen(result);

                connection.AmqpSettings.RuntimeProvider = this;
                connection.Closed += this.connection_Closed;
                lock (this.connections)
                {
                    this.connections.Add(connection.Identifier, connection);
                }
            }
            catch (Exception exception)
            {
                connection.SafeClose(exception);
            }
        }

        public AmqpConnection CreateConnection(TransportBase transport, ProtocolHeader protocolHeader, 
            bool isInitiator, AmqpSettings amqpSettings, AmqpConnectionSettings connectionSettings)
        {
            return new AmqpConnection(transport, protocolHeader, false, amqpSettings, connectionSettings);
        }

        public AmqpSession CreateSession(AmqpConnection connection, AmqpSessionSettings settings)
        {
            throw new InvalidOperationException();
        }

        public AmqpLink CreateLink(AmqpSession session, AmqpLinkSettings settings)
        {
            bool isReceiver = settings.Role.Value;
            AmqpLink link;
            if (isReceiver)
            {
                if (settings.Target is Target && ((Target)settings.Target).Dynamic())
                {
                    string name = string.Format("$dynamic.{0}", Interlocked.Increment(ref this.dynamicId));
                    this.queues.Add(name, new TestQueue(this));
                    ((Target)settings.Target).Address = name;
                }

                link = new ReceivingAmqpLink(session, settings);
            }
            else
            {
                if (((Source)settings.Source).Dynamic())
                {
                    string name = string.Format("$dynamic.{0}", Interlocked.Increment(ref this.dynamicId));
                    this.queues.Add(name, new TestQueue(this));
                    ((Source)settings.Source).Address = name;
                }

                link = new SendingAmqpLink(session, settings);
            }

            return link;
        }

        public IAsyncResult BeginOpenLink(AmqpLink link, TimeSpan timeout, AsyncCallback callback, object state)
        {
            if (link.IsReceiver && link.Settings.Target is Coordinator)
            {
                this.txnManager.AddCoordinator((ReceivingAmqpLink)link);
            }
            else
            {
                string address = link.IsReceiver ?
                    ((Target)link.Settings.Target).Address.ToString() :
                    ((Source)link.Settings.Source).Address.ToString();

                TestQueue queue;
                if (!this.queues.TryGetValue(address, out queue))
                {
                    if (!this.implicitQueue)
                    {
                        throw new AmqpException(AmqpErrorCode.NotFound, string.Format("Node '{0}' not found", address));
                    }

                    queue = new TestQueue(this);
                    this.queues.Add(address, queue);
                    link.Closed += (s, e) => this.queues.Remove(address);
                }

                queue.CreateClient(link);
            }

            return new CompletedAsyncResult(callback, state);
        }

        void ILinkFactory.EndOpenLink(IAsyncResult result)
        {
            CompletedAsyncResult.End(result);
        }

        static X509Certificate2 GetCertificate(string certFindValue)
        {
            StoreLocation[] locations = new StoreLocation[] { StoreLocation.LocalMachine, StoreLocation.CurrentUser };
            foreach (StoreLocation location in locations)
            {
                X509Store store = new X509Store(StoreName.My, location);
                store.Open(OpenFlags.OpenExistingOnly);

                X509Certificate2Collection collection = store.Certificates.Find(
                    X509FindType.FindBySubjectName,
                    certFindValue,
                    false);

                if (collection.Count == 0)
                {
                    collection = store.Certificates.Find(
                        X509FindType.FindByThumbprint,
                        certFindValue,
                        false);
                }

#if DOTNET_CORE
                store.Dispose();
#else
                store.Close();
#endif
                if (collection.Count > 0)
                {
                    return collection[0];
                }
            }

            throw new ArgumentException("No certificate can be found using the find value.");
        }

        sealed class CompletedAsyncResult : IAsyncResult
        {
            readonly object state;

            public CompletedAsyncResult(AsyncCallback callback, object state)
            {
                this.state = state;
                callback(this);
            }

            object IAsyncResult.AsyncState { get { return this.state; } }

            WaitHandle IAsyncResult.AsyncWaitHandle { get { throw new NotImplementedException(); } }

            bool IAsyncResult.CompletedSynchronously { get { return true; } }

            bool IAsyncResult.IsCompleted { get { return true; } }

            public static void End(IAsyncResult result)
            {
            }
        }

        sealed class TestPlainAuthenticator : ISaslPlainAuthenticator
        {
            static TestPlainAuthenticator defaultInstance = new TestPlainAuthenticator("guest", "guest");
            string userName;
            string password;

            public TestPlainAuthenticator(string userName, string password)
            {
                this.userName = userName;
                this.password = password;
            }

            public static TestPlainAuthenticator Default
            {
                get { return defaultInstance; }
            }

            Task<IPrincipal> ISaslPlainAuthenticator.AuthenticateAsync(string userName, string password)
            {
                if (!userName.Equals(this.userName, StringComparison.OrdinalIgnoreCase) &&
                    !password.Equals(this.password))
                {
                    throw new UnauthorizedAccessException("Invalid user name or password.");
                }

                IPrincipal principal = new GenericPrincipal(new GenericIdentity(userName), new string[] { "SEND", "LISTEN", "MANAGE" });
                return Task.FromResult(principal);
            }
        }

        sealed class BrokerMessage : AmqpMessage
        {
            readonly BufferListStream stream;

            public BrokerMessage(AmqpMessage message)
            {
                this.stream = (BufferListStream)message.ToStream();
                foreach (var buffer in message.RawByteBuffers) buffer.Clone();
                this.RawByteBuffers = message.RawByteBuffers;
            }

            public object LockedBy { get; set; }

            public LinkedListNode<BrokerMessage> Node { get; set; }

            public void Unlock()
            {
                this.LockedBy = null;
                this.DeliveryTag = AmqpConstants.NullBinary;
                this.DeliveryId = 0;
                this.State = null;
                this.StateChanged = false;
                this.Link = null;
                this.stream.Position = 0;
                this.PrepareForSend();
            }

            public override long SerializedMessageSize
            {
                get { return this.stream.Length; }
            }

            public override ArraySegment<byte>[] GetPayload(int payloadSize, out bool more)
            {
                ArraySegment<byte>[] segments = this.stream.ReadBuffers(payloadSize, false, out more);
                if (segments == null)
                {
                    return null;
                }

                // copy the buffer because the message could be disposed while the buffers could be
                // stilled queued in the transport. Need to fix the protocol stack to take ByteBuffer
                // so we can dispose the buffers after write operation completes.
                int size = 0;
                foreach (var segment in segments) size += segment.Count;
                byte[] buffer = new byte[size];
                size = 0;
                foreach (var segment in segments)
                {
                    Buffer.BlockCopy(segment.Array, segment.Offset, buffer, size, segment.Count);
                    size += segment.Count;
                }

                return new ArraySegment<byte>[] { new ArraySegment<byte>(buffer) };
            }

            protected override void OnCompletePayload(int payloadSize)
            {
                this.stream.Position += payloadSize;
            }
        }

        sealed class TestQueue
        {
            readonly TestAmqpBroker broker;
            readonly LinkedList<BrokerMessage> messages;
            readonly Queue<Consumer> waiters;
            readonly Dictionary<int, Publisher> publishers;
            readonly Dictionary<int, Consumer> consumers;
            readonly object syncRoot;
            int currentId;

            public TestQueue(TestAmqpBroker broker)
            {
                this.broker = broker;
                this.messages = new LinkedList<BrokerMessage>();
                this.waiters = new Queue<Consumer>();
                this.publishers = new Dictionary<int, Publisher>();
                this.consumers = new Dictionary<int, Consumer>();
                this.syncRoot = this.waiters;
            }

            public void CreateClient(AmqpLink link)
            {
                int id = Interlocked.Increment(ref this.currentId);
                if (link.IsReceiver)
                {
                    Publisher publisher = new Publisher(this, (ReceivingAmqpLink)link, id);
                    lock (this.publishers)
                    {
                        this.publishers.Add(id, publisher);
                    }
                }
                else
                {
                    Consumer consumer = new Consumer(this, (SendingAmqpLink)link, id);
                    lock (this.consumers)
                    {
                        this.consumers.Add(id, consumer);
                    }
                }
            }

            Consumer GetConsumerWithLock()
            {
                Consumer consumer = null;
                while (this.waiters.Count > 0)
                {
                    consumer = this.waiters.Peek();
                    if (consumer.Credit > 0)
                    {
                        consumer.Credit--;
                        if (consumer.Credit == 0)
                        {
                            this.waiters.Dequeue();
                        }

                        break;
                    }
                    else
                    {
                        this.waiters.Dequeue();
                        consumer = null;
                    }
                }

                return consumer;
            }

            void Enqueue(BrokerMessage message)
            {
                Consumer consumer = null;
                lock (this.syncRoot)
                {
                    consumer = this.GetConsumerWithLock();
                    if (consumer == null)
                    {
                        message.Node = this.messages.AddLast(message);
                    }
                    else if (consumer.SettleMode != SettleMode.SettleOnSend)
                    {
                        message.LockedBy = consumer;
                        message.Node = this.messages.AddLast(message);
                    }
                }

                if (consumer != null)
                {
                    consumer.Signal(message);
                }
            }

            void Dequeue(Consumer consumer, int credit)
            {
                List<BrokerMessage> messageList = new List<BrokerMessage>();
                lock (this.syncRoot)
                {
                    consumer.Credit += credit;

                    var current = this.messages.First;
                    while (current != null)
                    {
                        if (current.Value.LockedBy == null)
                        {
                            messageList.Add(current.Value);
                            if (consumer.SettleMode == SettleMode.SettleOnSend)
                            {
                                var temp = current;
                                current = current.Next;
                                this.messages.Remove(temp);
                            }
                            else
                            {
                                current.Value.LockedBy = consumer;
                            }

                            consumer.Credit--;
                            if (consumer.Credit == 0)
                            {
                                break;
                            }
                        }
                        else
                        {
                            current = current.Next;
                        }
                    }

                    if (consumer.Credit > 0)
                    {
                        this.waiters.Enqueue(consumer);
                    }
                }

                foreach (var m in messageList)
                {
                    consumer.Signal(m);
                }
            }

            public void Dequeue(BrokerMessage message)
            {
                lock (this.syncRoot)
                {
                    this.messages.Remove(message.Node);
                }
            }

            public void Unlock(BrokerMessage message)
            {
                Consumer consumer = null;
                lock (this.syncRoot)
                {
                    message.Unlock();
                    consumer = this.GetConsumerWithLock();
                    if (consumer != null)
                    {
                        if (consumer.SettleMode == SettleMode.SettleOnSend)
                        {
                            this.messages.Remove(message.Node);
                        }
                        else
                        {
                            message.LockedBy = consumer;
                        }
                    }
                }

                if (consumer != null)
                {
                    consumer.Signal(message);
                }
            }

            void OnConsumerClosed(int id, Consumer consumer)
            {
                lock (this.syncRoot)
                {
                    this.consumers.Remove(id);
                    var node = this.messages.First;
                    while (node != null)
                    {
                        var temp = node;
                        node = node.Next;
                        if (temp.Value.LockedBy == consumer)
                        {
                            this.Unlock(temp.Value);
                        }
                    }
                }
            }

            sealed class Publisher
            {
                readonly TestQueue queue;
                readonly ReceivingAmqpLink link;
                readonly int id;

                public Publisher(TestQueue queue, ReceivingAmqpLink link, int id)
                {
                    this.queue = queue;
                    this.link = link;
                    this.id = id;
                    this.link.RegisterMessageListener(this.OnMessage);
                    this.link.Closed += new EventHandler(link_Closed);
                }

                void link_Closed(object sender, EventArgs e)
                {
                    lock (this.queue.publishers)
                    {
                        this.queue.publishers.Remove(this.id);
                    }
                }

                void OnMessage(AmqpMessage message)
                {
                    string errorCondition = null;
                    if (message.ApplicationProperties != null &&
                        message.ApplicationProperties.Map.TryGetValue<string>("errorcondition", out errorCondition))
                    {
                        this.link.DisposeMessage(message, new Rejected() { Error = new Error() { Condition = errorCondition, Description = "message was rejected" } }, true, false);
                    }
                    else
                    {
                        if (message.TxnId.Array != null)
                        {
                            Transaction txn = this.queue.broker.txnManager.GetTransaction(message.TxnId);
                            txn.AddOperation(message, this.OnTxnDischarge);
                            this.link.DisposeMessage(message, new TransactionalState() { Outcome = AmqpConstants.AcceptedOutcome }, true, message.Batchable);
                        }
                        else
                        {
                            this.queue.Enqueue(new BrokerMessage(message));
                            this.link.AcceptMessage(message, message.Batchable);
                            message.Dispose();
                        }
                    }
                }

                void OnTxnDischarge(Delivery delivery, bool fail)
                {
                    if (!fail)
                    {
                        this.queue.Enqueue(new BrokerMessage((AmqpMessage)delivery));
                    }
                }
            }

            sealed class Consumer
            {
                readonly TestQueue queue;
                readonly SendingAmqpLink link;
                readonly int id;
                int tag;

                public Consumer(TestQueue queue, SendingAmqpLink link, int id)
                {
                    this.queue = queue;
                    this.link = link;
                    this.id = id;
                    this.link.Closed += new EventHandler(link_Closed);
                    this.link.RegisterCreditListener(this.OnCredit);
                    this.link.RegisterDispositionListener(this.OnDispose);
                }

                public int Credit { get; set; }

                public SettleMode SettleMode { get { return this.link.Settings.SettleType; } }

                public void Signal(BrokerMessage message)
                {
                    this.link.SendMessageNoWait(message, this.GetNextTag(), AmqpConstants.NullBinary);
                }

                ArraySegment<byte> GetNextTag()
                {
                    return new ArraySegment<byte>(BitConverter.GetBytes(Interlocked.Increment(ref this.tag)));
                }

                void link_Closed(object sender, EventArgs e)
                {
                    this.Credit = 0;
                    this.queue.OnConsumerClosed(this.id, this);
                }

                void OnCredit(uint credit, bool drain, ArraySegment<byte> txnId)
                {
                    this.queue.Dequeue(this, (int)credit);
                }

                void OnDispose(Delivery delivery)
                {
                    if (delivery.Transactional())
                    {
                        Transaction txn = this.queue.broker.txnManager.GetTransaction(((TransactionalState)delivery.State).TxnId);
                        txn.AddOperation(delivery, this.OnTxnDischarge);
                        this.link.DisposeDelivery(delivery, false, delivery.State, false);
                    }
                    else
                    {
                        if (!delivery.Settled)
                        {
                            this.link.DisposeDelivery(delivery, true, delivery.State);
                        }

                        BrokerMessage message = (BrokerMessage)delivery;
                        if (delivery.State.DescriptorCode == Accepted.Code ||
                            delivery.State.DescriptorCode == Rejected.Code ||
                            delivery.State.DescriptorCode == Modified.Code)
                        {
                            if (message.Node != null && message.Node.List != null)
                            {
                                this.queue.Dequeue(message);
                            }

                            delivery.Dispose();
                        }
                        else if (delivery.State.DescriptorCode == Released.Code)
                        {
                            this.queue.Unlock(message);
                        }
                    }
                }

                void OnTxnDischarge(Delivery delivery, bool fail)
                {
                    if (!fail)
                    {
                        BrokerMessage message = (BrokerMessage)delivery;
                        if (message.Node != null && message.Node.List != null)
                        {
                            this.queue.Dequeue(message);
                        }

                        this.link.DisposeDelivery(delivery, true, delivery.State);
                        delivery.Dispose();
                    }
                }
            }
        }

        sealed class Transaction
        {
            readonly Queue<Tuple<Delivery, Action<Delivery, bool>>> operations;

            public Transaction()
            {
                this.operations = new Queue<Tuple<Delivery, Action<Delivery, bool>>>();
            }

            public int Id { get; set; }

            public void AddOperation(Delivery delivery, Action<Delivery, bool> commit)
            {
                lock (this.operations)
                {
                    this.operations.Enqueue(Tuple.Create(delivery, commit));
                }
            }

            public void Discharge(bool fail)
            {
                foreach (var op in this.operations)
                {
                    op.Item2(op.Item1, fail);
                }
            }
        }

        sealed class TxnManager
        {
            readonly Dictionary<SequenceNumber, ReceivingAmqpLink> coordinators;
            readonly Dictionary<int, Transaction> transactions;
            int id;

            public TxnManager()
            {
                this.coordinators = new Dictionary<SequenceNumber, ReceivingAmqpLink>();
                this.transactions = new Dictionary<int, Transaction>();
            }

            public void AddCoordinator(ReceivingAmqpLink link)
            {
                link.RegisterMessageListener(this.OnMessage);
                link.Closed += this.link_Closed;
                lock (this.coordinators)
                {
                    this.coordinators.Add(link.Identifier, link);
                }
            }

            public Transaction GetTransaction(ArraySegment<byte> txnId)
            {
                int id = BitConverter.ToInt32(txnId.Array, txnId.Offset);
                return this.transactions[id];
            }

            void link_Closed(object sender, EventArgs e)
            {
                lock (this.coordinators)
                {
                    this.coordinators.Remove(((AmqpLink)sender).Identifier);
                }
            }

            void OnMessage(AmqpMessage message)
            {
                Outcome outcome;
                if (message.ValueBody.Value is Declare)
                {
                    int txnId = this.CreateTransaction();
                    outcome = new Declared() { TxnId = new ArraySegment<byte>(BitConverter.GetBytes(txnId)) };
                }
                else if (message.ValueBody.Value is Discharge)
                {
                    Discharge discharge = (Discharge)message.ValueBody.Value;
                    int txnId = BitConverter.ToInt32(discharge.TxnId.Array, discharge.TxnId.Offset);
                    Transaction txn;
                    if (this.transactions.TryGetValue(txnId, out txn))
                    {
                        lock (this.transactions)
                        {
                            this.transactions.Remove(txnId);
                        }

                        txn.Discharge(discharge.Fail ?? false);
                        outcome = AmqpConstants.AcceptedOutcome;
                    }
                    else
                    {
                        outcome = new Rejected() { Error = new Error() { Condition = AmqpErrorCode.NotFound } };
                    }
                }
                else
                {
                    outcome = new Rejected() { Error = new Error() { Condition = AmqpErrorCode.NotAllowed } };
                }

                message.Link.DisposeDelivery(message, true, outcome);
            }

            int CreateTransaction()
            {
                Transaction txn = new Transaction() { Id = Interlocked.Increment(ref this.id) };
                lock (this.transactions)
                {
                    this.transactions.Add(txn.Id, txn);
                    return txn.Id;
                }
            }
        }
    }
}
